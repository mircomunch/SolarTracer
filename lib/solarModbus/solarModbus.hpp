#include <ModbusMaster.h>
#include "../../include/solarModbus_setup.hpp"

class SolarModbus {
public:
    ModbusMaster node;
    static uint8_t _pin_re_de;
    uint8_t _pin_di;
    uint8_t _pin_ro;
    int _nIter = 2;
    uint8_t _modbus_id = 0x01;
    int _serial_speed = 9600;
    Stream &_serial = Serial2;
    HardwareSerial &_serial_stream = Serial2;

    SolarModbus(uint8_t pin_re_de, uint8_t pin_di, uint8_t pin_ro,
                int nIter, uint8_t modbus_id, int serial_speed,
                Stream &serial, HardwareSerial &serial_stream);
    uint8_t writeSingleCoil(uint16_t data, uint16_t address);
    uint8_t readCoils(uint16_t address, void *variable);
    uint8_t writeMultipleRegisters(uint16_t *data, uint16_t address, uint8_t nReg);
    uint8_t readHoldingRegisters(uint16_t address, void *variable, uint8_t nReg);
    uint8_t readInputRegisters(uint16_t address, void *variable, uint8_t nReg, bool oneHundredTimes);
    uint8_t readDiscreteInputs(uint16_t address, void *variable);
    uint8_t readSingleInputRegisters(uint16_t address, void *variable, bool oneHundredTimes);

    String exceptionDescription(uint8_t exception);
    // String exceptionHandler(uint8_t result, String mode, String address);
    void exceptionHandler(uint8_t result, String mode, String address);

private:
    static void setPin();
    static void resetPin();
};