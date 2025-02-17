#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <TimeLib.h>
#include <ArduinoJson.h>

#include <setup.hpp>
#include <solarModbus.hpp>
#include <DHT_temphum.hpp>
#include <MQTT.hpp>
#include <modbusAddress.hpp>

SolarModbus TRACER(PIN_RE_DE, PIN_DI, PIN_RO, READ_TENTATIVE, TRACER_MODBUS_ID, TRACER_SERIAL_STREAM_SPEED, TRACER_SERIAL_STREAM, TRACER_SERIAL_STREAM);
char _host_char[sizeof(MQTT_HOST)] = MQTT_HOST;

#ifdef MQTT_AUTH
char _host_user[sizeof(MQTT_USER)] = MQTT_USER;
char _host_psw[sizeof(MQTT_PASSWORD)] = MQTT_PASSWORD;
MQTT mqtt(BOARD_ID, _host_char, MQTT_PORT, MQTT_PUBLISH_MESSAGE_MAX_SIZE, MQTT_CERT_EN, _host_user, _host_psw);
#else
MQTT mqtt(BOARD_ID, _host_char, MQTT_PORT);
#endif

TimerHandle_t dataTimer;
TimerHandle_t loadTimer;
// TimerHandle_t publishTimer;
// TimerHandle_t readTracerTimer;
long mqttReconnectAttempt = 0;
long wifiReconnectAttempt = 0;

#ifdef SENS_TEMPHUM
DHTsensor dht;
// TimerHandle_t readTempHumTimer;
#endif

bool datetimeSetted = false;
bool cmdRun = false;
bool cmdLoad = false;
bool acq = false;
bool control = false;
// struct flags
// {
// 	bool readTracer = false;
// 	bool readDht = false;
// 	bool publishMqtt = false;
// 	bool controlLoad = false;
// } interruptsFlags;

struct SolarData
{
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

struct SolarSettings
{
	uint16_t batteryType[1] = {0x01};
	uint16_t batteryCapacity[1] = {0x07};
	uint16_t controllerSupply[1] = {0x01};
	uint16_t batteryVoltage[1] = {0x01};
	uint16_t outputMode = 0x0001;
	uint16_t loadOn = 0x0001;
	uint16_t loadOff = 0x0000;
} TracerSettings;

void readTracer(JsonArray *readArray);
void setupTracer();
#ifdef SENS_TEMPHUM
    void readTempHum(JsonArray *readArray);
#endif
void setDatetime(String datetime);
void mqttSubscribeCallback(char *topic, byte *payload, unsigned int length);
void mqttParseMessage(char *topic, const char *payloadStr);
void dataPublishCallback();
void loadControlCallback();

void setup()
{
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

	while (WiFi.status() != WL_CONNECTED)
	{
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

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        long now = millis();
        if (now - wifiReconnectAttempt > 5000)
        {
            wifiReconnectAttempt = now;
            #ifdef DEBUG
                SERIAL_DEBUG.println("Reconnecting to WiFi...");
            #endif
            if (WiFi.begin(WIFI_SSID, WIFI_PASSWORD) == WL_CONNECTED)
            {
                #ifdef DEBUG
                    SERIAL_DEBUG.println("Connected to: " + String(WIFI_SSID));
                #endif
                wifiReconnectAttempt = 0;
                return;
            }
        }
    }
	
    if (!mqtt.client.connected())
    {
        long now = millis();
        if (now - mqttReconnectAttempt > 5000)
        {
            mqttReconnectAttempt = now;
            if (mqtt.reconnect())
            {
                // Manage subscriptions
                if (mqtt.client.subscribe(MQTT_DATETIME_TOPIC, MQTT_QOS_SUB))
                {
                    #ifdef DEBUG
                        SERIAL_DEBUG.println("Subscribed to: " + String(MQTT_DATETIME_TOPIC));
                    #endif
                }
                if (mqtt.client.subscribe(MQTT_CMD_TOPIC, MQTT_QOS_SUB))
                {
                    #ifdef DEBUG
                        SERIAL_DEBUG.println("Subscribed to: " + String(MQTT_CMD_TOPIC));
                    #endif
                }
                mqtt.client.setCallback(mqttSubscribeCallback);
                mqttReconnectAttempt = 0;
                return;
            }
        }
    }
	mqtt.client.loop();

	if (acq)
	{
		acq = false;
		DynamicJsonDocument doc(2048);
		doc["id"] = BOARD_ID;
		doc["location"] = LOCATION;
		if (mqtt.datetimeSetted && mqtt.cmdRun)
		{
			JsonArray readings = doc.createNestedArray("readings");
			readTracer(&readings);

			#ifdef SENS_TEMPHUM
				readTempHum(&readings);
			#endif

			String message;
			serializeJson(doc, message);
			SERIAL_DEBUG.println(message);

			bool pubStatus = mqtt.publishMessage(MQTT_PUBLISH_TOPIC, message, false);
			if (!pubStatus)
			{
				#ifdef DEBUG
					SERIAL_DEBUG.println("--- PUBLISH ERROR OCCURRED ---");
				#endif
			}
			else
			{
				#ifdef DEBUG
					SERIAL_DEBUG.println("- MESSAGE PUBLISHED -");
				#endif
			}
		}
		xTimerStart(dataTimer, 0);
	}

	if (control)
	{
		uint8_t result;
		if (mqtt.cmdLoad)
		{
			// LOAD_ON
			result = TRACER.writeSingleCoil(TracerSettings.loadOn, MODBUS_ADDRESS_LOAD_MANUAL_ONOFF);
		}
		else
		{
			// LOAD_OFF
			result = TRACER.writeSingleCoil(TracerSettings.loadOff, MODBUS_ADDRESS_LOAD_MANUAL_ONOFF);
		}
		TRACER.exceptionHandler(result, "Write", "MODBUS_ADDRESS_LOAD_MANUAL_ONOFF");
		xTimerStart(loadTimer, 0);
	}
}

void setupTracer()
{
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
	if (result != 0)
	{
		TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_BATTERY_TYPE");
	}
	else
	{
		#ifdef DEBUG
				SERIAL_DEBUG.printf("BATTERY_TYPE: 0x%X\n", TracerReadings.batteryType);
		#endif
	}
	// READ BATTERY_CAPACITY
	result = TRACER.readHoldingRegisters(MODBUS_ADDRESS_BATTERY_CAPACITY, &TracerReadings.batteryCapacity, 1);
	if (result != 0)
	{
		TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_BATTERY_CAPACITY");
	}
	else
	{
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
	if (result != 0)
	{
		TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_BATTERY_RATED_VOLTAGE_CODE");
	}
	else
	{
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

void readTracer(JsonArray *readArray)
{
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
	if (result != 0)
	{
		TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_BATTERY_CURRENT_VOLTAGE");
	}
	else
	{
		JsonObject reading = readArray->createNestedObject();
		reading["data_id"] = BATT_V_ID;
		reading["unit"] = BATT_V_UNIT;
		JsonArray data = reading.createNestedArray("data");
		JsonObject singleData = data.createNestedObject();
		singleData["timestamp"] = (int)now();
		singleData["val"] = TracerReadings.batteryVoltage;
		#ifdef DEBUG
				SERIAL_DEBUG.printf("BATTERY_VOLTAGE: %.2f\n", TracerReadings.batteryVoltage);
		#endif
	}
	// READ BATTERY_CHARGE_CURRENT
	result = TRACER.readSingleInputRegisters(MODBUS_ADDRESS_BATTERY_CHARGE_CURRENT, &TracerReadings.batteryCurrent, true);
	if (result != 0)
	{
		TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_BATTERY_CHARGE_CURRENT");
	}
	else
	{
		JsonObject reading = readArray->createNestedObject();
		reading["data_id"] = BATT_C_ID;
		reading["unit"] = BATT_C_UNIT;
		JsonArray data = reading.createNestedArray("data");
		JsonObject singleData = data.createNestedObject();
		singleData["timestamp"] = (int)now();
		singleData["val"] = TracerReadings.batteryCurrent;
		#ifdef DEBUG
				SERIAL_DEBUG.printf("BATTERY_CURRENT: %.2f\n", TracerReadings.batteryCurrent);
		#endif
	}
	// READ BATTERY_SOC
	result = TRACER.readSingleInputRegisters(MODBUS_ADDRESS_BATTERY_SOC, &TracerReadings.batterySOC, false);
	if (result != 0)
	{
		TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_BATTERY_SOC");
	}
	else
	{
		JsonObject reading = readArray->createNestedObject();
		reading["data_id"] = BATT_SOC_ID;
		reading["unit"] = BATT_SOC_UNIT;
		JsonArray data = reading.createNestedArray("data");
		JsonObject singleData = data.createNestedObject();
		singleData["timestamp"] = (int)now();
		singleData["val"] = TracerReadings.batterySOC;
		#ifdef DEBUG
				SERIAL_DEBUG.printf("BATTERY_SOC: %.2f\n", TracerReadings.batterySOC);
		#endif
	}
	// READ LOAD_VOLTAGE
	result = TRACER.readSingleInputRegisters(MODBUS_ADDRESS_LOAD_VOLTAGE, &TracerReadings.loadVoltage, true);
	if (result != 0)
	{
		TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_LOAD_VOLTAGE");
	}
	else
	{
		JsonObject reading = readArray->createNestedObject();
		reading["data_id"] = LOAD_V_ID;
		reading["unit"] = LOAD_V_UNIT;
		JsonArray data = reading.createNestedArray("data");
		JsonObject singleData = data.createNestedObject();
		singleData["timestamp"] = (int)now();
		singleData["val"] = TracerReadings.loadVoltage;
		#ifdef DEBUG
				SERIAL_DEBUG.printf("LOAD_VOLTAGE: %.2f\n", TracerReadings.loadVoltage);
		#endif
	}
	// READ LOAD_CURRENT
	result = TRACER.readSingleInputRegisters(MODBUS_ADDRESS_LOAD_CURRENT, &TracerReadings.loadCurrent, true);
	if (result != 0)
	{
		TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_LOAD_CURRENT");
	}
	else
	{
		JsonObject reading = readArray->createNestedObject();
		reading["data_id"] = LOAD_C_ID;
		reading["unit"] = LOAD_C_UNIT;
		JsonArray data = reading.createNestedArray("data");
		JsonObject singleData = data.createNestedObject();
		singleData["timestamp"] = (int)now();
		singleData["val"] = TracerReadings.loadCurrent;
		#ifdef DEBUG
				SERIAL_DEBUG.printf("LOAD_CURRENT: %.2f\n", TracerReadings.loadCurrent);
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
	if (result != 0)
	{
		TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_PV_VOLTAGE");
	}
	else
	{
		JsonObject reading = readArray->createNestedObject();
		reading["data_id"] = PV_V_ID;
		reading["unit"] = PV_V_UNIT;
		JsonArray data = reading.createNestedArray("data");
		JsonObject singleData = data.createNestedObject();
		singleData["timestamp"] = (int)now();
		singleData["val"] = TracerReadings.pvVoltage;
		#ifdef DEBUG
				SERIAL_DEBUG.printf("PV_VOLTAGE: %.2f\n", TracerReadings.pvVoltage);
		#endif
	}
	// READ PV_CURRENT
	result = TRACER.readSingleInputRegisters(MODBUS_ADDRESS_PV_CURRENT, &TracerReadings.pvCurrent, true);
	if (result != 0)
	{
		TRACER.exceptionHandler(result, "Read", "MODBUS_ADDRESS_PV_CURRENT");
	}
	else
	{
		JsonObject reading = readArray->createNestedObject();
		reading["data_id"] = PV_C_ID;
		reading["unit"] = PV_C_UNIT;
		JsonArray data = reading.createNestedArray("data");
		JsonObject singleData = data.createNestedObject();
		singleData["timestamp"] = (int)now();
		singleData["val"] = TracerReadings.pvCurrent;
		#ifdef DEBUG
				SERIAL_DEBUG.printf("PV_CURRENT: %.2f\n", TracerReadings.pvCurrent);
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

void readTempHum(JsonArray *readArray)
{
	JsonObject env_temp = readArray->createNestedObject();
	env_temp["data_id"] = TEMP_ID;
	env_temp["unit"] = TEMP_UNIT;
	JsonArray temp = env_temp.createNestedArray("data");
	JsonObject singleTemp = temp.createNestedObject();
	singleTemp["timestamp"] = (int)now();
	singleTemp["val"] = dht.readTemperature();

	JsonObject env_hum = readArray->createNestedObject();
	env_hum["data_id"] = HUM_ID;
	env_hum["unit"] = HUM_UNIT;
	JsonArray hum = env_hum.createNestedArray("data");
	JsonObject singleHum = hum.createNestedObject();
	singleHum["timestamp"] = (int)now();
	singleHum["val"] = dht.readHumidity();
}

void setDatetime(String datetime)
{
    uint64_t datetimeD = datetime.toDouble();
    if (datetimeD > 0)
    {
        setTime(datetimeD);
    }
    datetimeSetted = true;
}

void mqttSubscribeCallback(char *topic, byte *payload, unsigned int length)
{
    #ifdef DEBUG
        SERIAL_DEBUG.print("Message arrived [");
        SERIAL_DEBUG.print(topic);
        SERIAL_DEBUG.print("]: ");
    #endif
    String payloadStr;
    for (int i = 0; i < length; i++)
    {
        payloadStr += (char)payload[i];
    }
    #ifdef DEBUG
        SERIAL_DEBUG.println(payloadStr);
    #endif

    mqttParseMessage(topic, payloadStr.c_str());
}

void mqttParseMessage(char *topic, const char *payloadStr)
{
    if (strcmp(topic, MQTT_DATETIME_TOPIC) == 0)
    {
        setDatetime(payloadStr);
    }
    if (strcmp(topic, MQTT_CMD_TOPIC) == 0)
    {
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payloadStr);
        JsonArray array = doc.as<JsonArray>();
        for (JsonVariant v : array)
        {
            DynamicJsonDocument elem(1024);
            deserializeJson(elem, v.as<String>());
            if (strcmp(elem["id"], BOARD_ID) == 0)
            {
                cmdRun = elem["run"];
                cmdLoad = elem["load"];
            }
            elem.clear();
        }
        // OLD VERSION
        // if (strcmp(doc["id"], BOARD_ID) == 0) {
        //     this->cmdRun = doc["run"];
        //     this->cmdLoad = doc["load"];
        // }
        doc.clear();
    }
}

void dataPublishCallback()
{
    acq = true;
}

void loadControlCallback()
{
    control = true;
}