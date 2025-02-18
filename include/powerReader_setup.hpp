// Serial debug
#define DEBUG
#ifdef DEBUG
    #define SERIAL_DEBUG Serial
    #define SERIAL_DEBUG_SPEED 115200
#endif

// JSY-MK-194G pin
#define JSY_PIN_RX 22 // RX
#define JSY_PIN_TX 23 // TX
#define JSY_CLAMP 1

// JSY-MK-194G Modbus serial comunication
#define JSY_SERIAL_STREAM Serial1
#define JSY_SERIAL_STREAM_SPEED 4800
#define JSY_MODBUS_ID 0x01

// Transmission data id
#define INV_V_ID "inv_voltage"
#define INV_V_UNIT "V"
#define INV_C_ID "inv_current"
#define INV_C_UNIT "A"
#define INV_POW_ID "inv_power"
#define INV_POW_UNIT "W"
#define INV_ENER_ID "inv_energy"
#define INV_ENER_UNIT "kWh"
#define INV_PF_ID "inv_powerFactor"
#define INV_PF_UNIT ""
#define INV_PD_ID "inv_powerDirection"
#define INV_PD_UNIT ""
#define INV_F_ID "inv_frequency"
#define INV_F_UNIT "Hz"

#define POWER_N_READINGS 7