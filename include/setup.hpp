#include "solarModbus_setup.hpp"
#include "dht11_setup.hpp"

// Serial debug
#define DEBUG
#ifdef DEBUG
    #define SERIAL_DEBUG Serial
    #define SERIAL_DEBUG_SPEED 115200
#endif

// BOARD NAME
#define BOARD_ID "LL-Parma"
#define LOCATION "ITA-Parma-UNIPR"

// Sensors activated
#define SENS_TEMPHUM

#ifdef SENS_TEMPHUM
    #define READINGS_N SOLAR_N_READINGS + TEMPHUM_N_READINGS
#else
    #define READINGS_N SOLAR_N_READINGS
#endif

// WiFI configurations
// #define WIFI_SSID "Greater"
// #define WIFI_PASSWORD "greater23"
#define WIFI_SSID "Batcaverna"
#define WIFI_PASSWORD "Spiderman"

// MQTT configurations
#define MQTT_AUTH
#ifdef MQTT_AUTH
	#define MQTT_HOST "350fd0725fa14a069e04d387121e69f7.s2.eu.hivemq.cloud"
	#define MQTT_PORT 8883
	#define MQTT_USER "admin"
	#define MQTT_PASSWORD "Admin123"
	#define MQTT_CERT_EN true // define if certificate needed for broker connection
#else
	#define MQTT_HOST "broker.hivemq.com"
	#define MQTT_PORT 1883
#endif

#define MQTT_TOPIC "living-labs/"
#define MQTT_PUBLISH_TOPIC MQTT_TOPIC "data"
#define MQTT_DATETIME_TOPIC MQTT_TOPIC "datetime"
#define MQTT_CMD_TOPIC MQTT_TOPIC "cmd"
#define MQTT_QOS_SUB 1
#define MQTT_PUBLISH_MESSAGE_MAX_SIZE 65536 / 2 // MQTT publish max size = 256MB

// Timer configurations
#define DATA_TIMER 5000
#define LOAD_TIMER 1000

//#define SOLAR_TIMER 5000
//#define DHT_TIMER 5000
//#define PUBLISH_TIMER 10000