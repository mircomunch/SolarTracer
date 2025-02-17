#ifndef POWERREADER_H
#define POWERREADER_H

#include <Arduino.h>
#include <ModbusMaster.h>

class PowerReader {
public:
    PowerReader(uint8_t txPin, uint8_t rxPin, uint32_t baudRate, uint8_t slaveID,
                int nIter, Stream &serial, HardwareSerial &serial_stream);

    void begin();

    uint8_t test();
    uint8_t readVoltage(int channel, float *variable);
    uint8_t readCurrent(int channel, float *variable);
    uint8_t readPower(int channel, float *variable);
    uint8_t readEnergy(int channel, float *variable);
    uint8_t readPowerfactor(int channel, float *variable);
    uint8_t readPowerdirection(float *variable);
    uint8_t readFrequency(float *variable);

    String exceptionDescription(uint8_t exception);

private:
    uint8_t readModbusRegister(uint16_t reg, float divisor, float *variable);

    Stream &_serial = Serial1;
    HardwareSerial &_serialPort = Serial1;
    ModbusMaster _modbusNode;
    uint8_t _txPin;
    uint8_t _rxPin;
    uint32_t _baudRate = 4800;
    uint8_t _slaveID;

    int _nIter = 2;
};

#endif
