// Serial debug
#define DEBUG
#ifdef DEBUG
    #define SERIAL_DEBUG Serial
    #define SERIAL_DEBUG_SPEED 115200
#endif

// MAX 485 pin DE, not(RE) connected togheter
#define PIN_RE_DE 27

// BOARD NAME
#define BOARD_NAME "digilent"

// Sensors activated
// #define CURRENT
#define TEMPERATURE

// DHT configurations
#define DHTPIN 13
#define DHTTYPE DHT11

// WiFI configurations
// #define WIFI_SSID "Greater"
// #define WIFI_PASSWORD "greater23"
#define WIFI_SSID "BATCAVE"
#define WIFI_PASSWORD "bathello01"

// MQTT configurations
// #define MQTT_HOST "broker.hivemq.com"
// #define MQTT_HOST "test.mosquitto.org"
// #define MQTT_PORT 1883
#define MQTT_HOST "350fd0725fa14a069e04d387121e69f7.s2.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_TOPIC "unipr/lab/"
#define MQTT_PUBLISH_TOPIC MQTT_TOPIC BOARD_NAME
#define MQTT_DATETIME_TOPIC MQTT_TOPIC "datetime"
#define MQTT_CMD_TOPIC MQTT_TOPIC "cmd"
#define MQTT_USER "admin"
#define MQTT_PASSWORD "Admin123"
#define CERT // define if certificate needed for broker connection

// Message configurations
#define DATA_TIMER 2000
#define LOAD_TIMER 1000