#include "solarModbus_setup.hpp"
#include "dht11_setup.hpp"

// Serial debug
#define DEBUG
#ifdef DEBUG
    #define SERIAL_DEBUG Serial
    #define SERIAL_DEBUG_SPEED 115200
#endif

// BOARD NAME
#define BOARD_ID "LL-HOME.US-1"
#define LOCATION "IT-Parma-Unipr"

// Sensors activated
#define SENS_TEMPHUM

#ifdef SENS_TEMPHUM
    #define READINGS_N SOLAR_N_READINGS + TEMPHUM_N_READINGS
#else
    #define READINGS_N SOLAR_N_READINGS
#endif

// WiFI configurations
#define WIFI_SSID "Greater"
#define WIFI_PASSWORD "greater23"
// #define WIFI_SSID "BATCAVE"
// #define WIFI_PASSWORD "bathello01"

// MQTT configurations
// #define MQTT_HOST "broker.hivemq.com"
// #define MQTT_HOST "test.mosquitto.org"
// #define MQTT_PORT 1883
#define MQTT_HOST "350fd0725fa14a069e04d387121e69f7.s2.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_TOPIC "living-labs/"
#define MQTT_PUBLISH_TOPIC MQTT_TOPIC "data"
#define MQTT_DATETIME_TOPIC MQTT_TOPIC "datetime"
#define MQTT_CMD_TOPIC MQTT_TOPIC "cmd"
#define MQTT_USER "admin"
#define MQTT_PASSWORD "Admin123"
#define CERT // define if certificate needed for broker connection

// Timer configurations
#define DATA_TIMER 2000
#define LOAD_TIMER 1000

//SOLAR_TIMER
//DHT_TIMER
//PUBLISH_TIMER