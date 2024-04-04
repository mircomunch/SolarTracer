#include <header.hpp>
#include <setup.hpp>
#include <solarModbus.hpp>
#include "./sensors/DHT_temphum/DHT_temphum.hpp"
#include "./MQTT/MQTT.hpp"

SolarModbus TRACER;

#ifdef SENS_TEMPHUM
  DHTsensor dht;
#endif

MQTT mqtt;
TimerHandle_t dataTimer;
TimerHandle_t loadTimer;

bool acq = false;
struct flags {
  bool readTracer = false;
  bool readDht = false;
  bool publishMqtt = false;
} intFlags;

struct singleData {
  int timestamp;
  float val;
};

struct dataFormat {
  String data_id;
  String unit;
  singleData data [10]; //define a maximum n of data transmitted -> to be improved 
};

struct readingsData {
  dataFormat readings [READINGS_N];
} readingsQueue;

struct temphumData {
  dataFormat temperature;
  dataFormat humidity;
} temphumReadings;

// temphumReadings.temperature.data_id = TEMP_ID;
// temphumReadings.temperature.unit = TEMP_UNIT;
struct SensorsData {
  float temperature;
  float humidity;
} SensorsReadings;

struct SolarData {
  uint16_t batteryType;
  uint16_t batteryCapacity;
  uint16_t controllerSupply;
  uint16_t batteryRatedVoltageCode;
  uint16_t outputMode;

  uint16_t batteryRatedVoltage;
  uint16_t batteryTemperature;
  float batteryVoltage;
  float batteryCurrent;
  float batterySOC;
  float batteryMaxVoltageToday;
  uint16_t batteryStatus;

  float pvVoltage;
  float pvCurrent;

  float loadVoltage;
  float loadCurrent;
  uint16_t loadPower;
} TracerReadings;

struct SolarSettings {
  uint16_t batteryType[1] = {0x01};
  uint16_t batteryCapacity[1] = {0x07};
  uint16_t controllerSupply[1] = {0x01};
  uint16_t batteryVoltage[1] = {0x01};
  uint16_t outputMode = 0x0001;
  uint16_t loadOn = 0x0001;
  uint16_t loadOff = 0x0000;
} TracerSettings;

void readTracer();
void setupTracer();
void readTempHum();
String composeMessage();

void dataPublishCallback() {
  acq = true;
}

void loadControlCallback() {
  uint8_t result;
  if (mqtt.cmdLoad) {
    // LOAD_ON
    result = TRACER.writeSingleCoil(TracerSettings.loadOn, MODBUS_ADDRESS_LOAD_MANUAL_ONOFF);
  } else {
    // LOAD_OFF
    result = TRACER.writeSingleCoil(TracerSettings.loadOff, MODBUS_ADDRESS_LOAD_MANUAL_ONOFF);
  }
  TRACER.exceptionHandler(result, "Write", "MODBUS_ADDRESS_LOAD_MANUAL_ONOFF");
  xTimerStart(loadTimer, 0);
}

void setup() {
  #ifdef DEBUG
    SERIAL_DEBUG.begin(SERIAL_DEBUG_SPEED);
  #endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  dataTimer = xTimerCreate("dataTimer", pdMS_TO_TICKS(DATA_TIMER),
                               pdFALSE, (void *)0,
                               reinterpret_cast<TimerCallbackFunction_t>(dataPublishCallback));
  loadTimer = xTimerCreate("loadTimer", pdMS_TO_TICKS(LOAD_TIMER),
                           pdFALSE, (void *)0,
                           reinterpret_cast<TimerCallbackFunction_t>(loadControlCallback));

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    #ifdef DEBUG
      SERIAL_DEBUG.print(".");
    #endif
  }
  #ifdef DEBUG
    SERIAL_DEBUG.println("\nWiFi connected\nIP address: ");
    SERIAL_DEBUG.println(WiFi.localIP());
    SERIAL_DEBUG.println("Init...");
  #endif

  /* SETUP TRACER */
  setupTracer();

  #ifdef DEBUG
    SERIAL_DEBUG.println("---------------");
  #endif

  /* START TIMERS */
  xTimerStart(dataTimer, 0);
  xTimerStart(loadTimer, 0);
}

void loop() {
  if (!mqtt.client.connected())
    mqtt.reconnect();
  mqtt.client.loop();

  if(acq) {
    acq = false;
    if (mqtt.datetimeSetted && mqtt.cmdRun) {
      readTracer();
      readTempHum();
      String message = composeMessage();
      // #ifdef DEBUG
      //   SERIAL_DEBUG.println(message);
      //   SERIAL_DEBUG.printf("TEMP: %.2f°C | HUM: %.2f%\n", SensorsReadings.temperature, SensorsReadings.humidity);
      // #endif
      bool pubStatus = mqtt.publishMessage(MQTT_PUBLISH_TOPIC, message, false);
      if (!pubStatus) {
        #ifdef DEBUG
          SERIAL_DEBUG.println("--- PUBLISH ERROR OCCURRED ---");
        #endif
      } else {
        #ifdef DEBUG
          SERIAL_DEBUG.println("- MESSAGE PUBLISHED -");
        #endif
      }
    }
    xTimerStart(dataTimer, 0);
  }
}

void setupTracer() {
  uint8_t result;

  // BATTERY_TYPE
  result = TRACER.writeMultipleRegisters(TracerSettings.batteryType, MODBUS_ADDRESS_BATTERY_TYPE, 1);
  TRACER.exceptionHandler(result, "Write", "MODBUS_ADDRESS_BATTERY_TYPE");
  // BATTERY_CAPACITY
  result = TRACER.writeMultipleRegisters(TracerSettings.batteryCapacity, MODBUS_ADDRESS_BATTERY_CAPACITY, 1);
  TRACER.exceptionHandler(result, "Write", "MODBUS_ADDRESS_BATTERY_CAPACITY");
  // // CONTROLLER_SUPPLY
  // result = TRACER.writeMultipleRegisters(TracerSettings.controllerSupply, MODBUS_ADDRESS_CONTROLLER_SUPPLY, 1);
  // TRACER.exceptionHandler(result, "Write", "MODBUS_ADDRESS_CONTROLLER_SUPPLY");
  // BATTERY_RATED_VOLTAGE
  result = TRACER.writeMultipleRegisters(TracerSettings.batteryVoltage, MODBUS_ADDRESS_BATTERY_RATED_VOLTAGE_CODE, 1);
  TRACER.exceptionHandler(result, "Write", "MODBUS_ADDRESS_BATTERY_RATED_VOLTAGE");
  // // LOAD_AUTOMATIC_ONOFF
  // result = TRACER.writeSingleCoil(TracerSettings.outputMode, MODBUS_ADDRESS_LOAD_AUTOMATIC_ONOFF);
  // TRACER.exceptionHandler(result, "Write", "MODBUS_ADDRESS_LOAD_AUTOMATIC_ONOFF");

  // READ VALUES - SETTINGS
  // READ BATTERY_TYPE
  result = TRACER.readHoldingRegisters(MODBUS_ADDRESS_BATTERY_TYPE, &TracerReadings.batteryType, 1);
  if (result != 0) {
    TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_BATTERY_TYPE");
  } else {
    #ifdef DEBUG
      SERIAL_DEBUG.printf("BATTERY_TYPE: 0x%X\n", TracerReadings.batteryType);
    #endif
  }
  // READ BATTERY_CAPACITY
  result = TRACER.readHoldingRegisters(MODBUS_ADDRESS_BATTERY_CAPACITY, &TracerReadings.batteryCapacity, 1);
  if (result != 0) {
    TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_BATTERY_CAPACITY");
  } else {
    #ifdef DEBUG
      SERIAL_DEBUG.printf("BATTERY_CAPACITY: 0x%X\n", TracerReadings.batteryCapacity);
    #endif
  }
  // // READ CONTROLLER_SUPPLY
  // result = TRACER.readHoldingRegisters(MODBUS_ADDRESS_CONTROLLER_SUPPLY, &TracerReadings.controllerSupply, 1);
  // if (result != 0) {
  //   TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_CONTROLLER_SUPPLY");
  // } else {
  //   #ifdef DEBUG
  //    SERIAL_DEBUG.printf("CONTROLLER_SUPPLY: 0x%X\n", TracerReadings.controllerSupply);
  //   #endif
  // }
  // READ BATTERY_RATED_VOLTAGE_CODE
  result = TRACER.readHoldingRegisters(MODBUS_ADDRESS_BATTERY_RATED_VOLTAGE_CODE, &TracerReadings.batteryRatedVoltageCode, 1);
  if (result != 0) {
    TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_BATTERY_RATED_VOLTAGE_CODE");
  } else {
    #ifdef DEBUG
      SERIAL_DEBUG.printf("BATTERY_RATED_VOLTAGE_CODE: 0x%X\n", TracerReadings.batteryRatedVoltageCode);
    #endif
  }
  // // READ LOAD_AUTO_ONOFF
  // result = TRACER.readCoils(MODBUS_ADDRESS_LOAD_AUTOMATIC_ONOFF, &TracerReadings.outputMode);
  // if (result != 0) {
  //     TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_LOAD_AUTOMATIC_ONOFF");
  // } else {
  //   #ifdef DEBUG
  //     SERIAL_DEBUG.printf("LOAD_AUTO_ONOFF: 0x%X\n", TracerReadings.outputMode);
  //   #endif
  // }
}

void readTracer(){
  uint8_t result;

  // READ VALUES - REAL-TIME
  // // READ BATTERY_TEMP
  // result = TRACER.readSingleInputRegisters(MODBUS_ADDRESS_BATTERY_TEMP, &TracerReadings.batteryTemperature, true);
  // if (result != 0) {
  //   TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_BATTERY_TEMP");
  // } else {
    // #ifdef DEBUG
    //   SERIAL_DEBUG.printf("BATTERY_TEMP: %f\n", TracerReadings.batteryTemperature);
    // #endif
  // }
  // READ BATTERY_CURRENT_VOLTAGE
  result = TRACER.readSingleInputRegisters(MODBUS_ADDRESS_BATTERY_CURRENT_VOLTAGE, &TracerReadings.batteryVoltage, true);
  if (result != 0) {
    TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_BATTERY_CURRENT_VOLTAGE");
  } else {
    #ifdef DEBUG
      SERIAL_DEBUG.printf("BATTERY_VOLTAGE: %f\n", TracerReadings.batteryVoltage);
    #endif
  }
  // READ BATTERY_CHARGE_CURRENT
  result = TRACER.readSingleInputRegisters(MODBUS_ADDRESS_BATTERY_CHARGE_CURRENT, &TracerReadings.batteryCurrent, true);
  if (result != 0) {
    TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_BATTERY_CHARGE_CURRENT");
  } else {
    #ifdef DEBUG
      SERIAL_DEBUG.printf("BATTERY_CURRENT: %f\n", TracerReadings.batteryCurrent);
    #endif
  }
  // READ BATTERY_SOC
  result = TRACER.readSingleInputRegisters(MODBUS_ADDRESS_BATTERY_SOC, &TracerReadings.batterySOC, false);
  if (result != 0) {
    TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_BATTERY_SOC");
  } else {
    #ifdef DEBUG
      SERIAL_DEBUG.printf("BATTERY_SOC: %f\n", TracerReadings.batterySOC);
    #endif
  }
  // READ LOAD_VOLTAGE
  result = TRACER.readSingleInputRegisters(MODBUS_ADDRESS_LOAD_VOLTAGE, &TracerReadings.loadVoltage, true);
  if (result != 0) {
    TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_LOAD_VOLTAGE");
  } else {
    #ifdef DEBUG
      SERIAL_DEBUG.printf("LOAD_VOLTAGE: %f\n", TracerReadings.loadVoltage);
    #endif
  }
  // READ LOAD_CURRENT
  result = TRACER.readSingleInputRegisters(MODBUS_ADDRESS_LOAD_CURRENT, &TracerReadings.loadCurrent, true);
  if (result != 0) {
    TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_LOAD_CURRENT");
  } else {
    #ifdef DEBUG
      SERIAL_DEBUG.printf("LOAD_CURRENT: %f\n", TracerReadings.loadCurrent);
    #endif
  }
  // // READ LOAD_POWER
  // result = TRACER.readInputRegisters(MODBUS_ADDRESS_LOAD_POWER, &TracerReadings.loadPower, 2, false);
  // if (result != 0) {
  //   TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_LOAD_POWER");
  // } else {
    // #ifdef DEBUG
    //   SERIAL_DEBUG.printf("LOAD_POWER: %x\n", TracerReadings.loadPower);
    // #endif
  // }
  // READ PV_VOLTAGE
  result = TRACER.readSingleInputRegisters(MODBUS_ADDRESS_PV_VOLTAGE, &TracerReadings.pvVoltage, true);
  if (result != 0) {
    TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_PV_VOLTAGE");
  } else {
    #ifdef DEBUG
      SERIAL_DEBUG.printf("PV_VOLTAGE: %f\n", TracerReadings.pvVoltage);
    #endif
  }
  // READ PV_CURRENT
  result = TRACER.readSingleInputRegisters(MODBUS_ADDRESS_PV_CURRENT, &TracerReadings.pvCurrent, true);
  if (result != 0) {
    TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_PV_CURRENT");
  } else {
    #ifdef DEBUG
      SERIAL_DEBUG.printf("PV_CURRENT: %f\n", TracerReadings.pvCurrent);
    #endif
  }
  // // READ BATTERY_MAX_VOLTAGE_TODAY
  // result = TRACER.readSingleInputRegisters(MODBUS_ADDRESS_STAT_MAX_BATTERY_VOLTAGE_TODAY, &TracerReadings.batteryMaxVoltageToday, true);
  // if (result != 0) {
  //   TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_STAT_MAX_BATTERY_VOLTAGE_TODAY");
  // } else {
    // #ifdef DEBUG
    //   SERIAL_DEBUG.printf("BATTERY_MAX_VOLTAGE: %f\n", TracerReadings.batteryMaxVoltageToday); ///FIXXXXXX
    // #endif
  // }
  // // READ BATTERY_STATUS
  // result = TRACER.readSingleInputRegisters(MODBUS_ADDRESS_BATTERY_STATUS, &TracerReadings.batteryStatus, false);
  // if (result != 0) {
  //   TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_BATTERY_STATUS");
  // } else {
    // #ifdef DEBUG
    //   SERIAL_DEBUG.printf("BATTERY_STATUS: %d\n", TracerReadings.batteryStatus / 100);
    // #endif
  // }
}

void readTempHum() {
  #ifdef SENS_TEMPHUM
    SensorsReadings.temperature = dht.readTemperature();
    SensorsReadings.humidity = dht.readHumidity();
    // singleData data;
    // data.timestamp = (int)now();
    // data.val = dht.readTemperature();
    // dhtReadings.temperature.data.append(data);
    // data.timestamp = (int)now();
    // data.val = dht.readHumidity();
    // dhtReadings.humidity.data.append(data);
  #endif
}

String composeMessage() {
  // StaticJsonDocument<200> obj;
  // String message;
  // obj["id"] = BOARD_ID;
  // obj["location"] = LOCATION;
  // obj["readings"] = readingsQueue;

  // serializeJson(obj, message);

  String message;
  message = "{\"id\":\"LL.1-US.1\",";
  message += "\"location\":\"IT-Parma-Unipr\",";
  message += "\"timestamp\":";
  message += (int)now();
  message += "000,";
  message += "\"sensors\":[";
  message += "{\"battery\":{";
  message += "\"voltage\":";
  message += TracerReadings.batteryVoltage;
  message += ",\"current\":";
  message += TracerReadings.batteryCurrent;
  message += ",\"soc\":";
  message += TracerReadings.batterySOC;
  message += "}},";
  message += "{\"load\":{";
  message += "\"voltage\":";
  message += TracerReadings.loadVoltage;
  message += ",\"current\":";
  message += TracerReadings.loadCurrent;
  // message += ",\"power\":";
  // message += TracerReadings.loadPower;
  message += "}},";
  message += "{\"pv\":{";
  message += "\"voltage\":";
  message += TracerReadings.pvVoltage;
  message += ",\"current\":";
  message += TracerReadings.pvCurrent;
  // #ifdef TEMPERATURE
    // message += "}},";
    // message += "{\"dht\":{";
    // message += "\"temperature\":";
    // message += SensorsReadings.temperature;
    // message += ",\"humidity\":";
    // message += SensorsReadings.humidity;
  // #endif
  message += "}}]";

  // message += "],";
  // message += "\"controller\":{\"err\":";
  // message += error;
  // message += "}";
  message += "}";

  return message;
}