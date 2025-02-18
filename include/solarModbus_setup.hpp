// Serial debug
#define DEBUG
#ifdef DEBUG
    #define SERIAL_DEBUG Serial
    #define SERIAL_DEBUG_SPEED 115200
#endif

// MAX 485 pin DE, not(RE) connected togheter
#define PIN_RE_DE 27
#define PIN_RO 16 // RX
#define PIN_DI 17 // TX

// TRACER Modbus serial comunication
#define TRACER_SERIAL_STREAM Serial2
#define TRACER_SERIAL_STREAM_SPEED 115200
#define TRACER_MODBUS_ID 0x01

// Transmission data id
#define BATT_V_ID "batt_voltage"
#define BATT_V_UNIT "V"
#define BATT_C_ID "batt_current"
#define BATT_C_UNIT "A"
#define BATT_SOC_ID "batt_soc"
#define BATT_SOC_UNIT "%"
#define PV_V_ID "pv_voltage"
#define PV_V_UNIT "V"
#define PV_C_ID "pv_current"
#define PV_C_UNIT "A"
#define LOAD_V_ID "load_voltage"
#define LOAD_V_UNIT "V"
#define LOAD_C_ID "load_current"
#define LOAD_C_UNIT "A"

#define SOLAR_N_READINGS 7