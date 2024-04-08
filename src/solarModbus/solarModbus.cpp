#include <Arduino.h>
#include <solarModbus.hpp>
#include "solarModbus_setup.hpp"

SolarModbus::SolarModbus(Stream &serial) {
    pinMode(PIN_RE_DE, OUTPUT);
    digitalWrite(PIN_RE_DE, LOW);

    SERIAL_STREAM.begin(SERIAL_STREAM_SPEED);
    this->node.begin(MODBUS_ID, serial);
    this->node.preTransmission(setPin);
    this->node.postTransmission(resetPin);

    #if defined(SERIAL_DEBUG)
        SERIAL_DEBUG.begin(SERIAL_DEBUG_SPEED);
    #endif // SERIAL_DEBUG
}

String SolarModbus::exceptionDescription(uint8_t exception) {
    String exceptionDescription = "";
    switch (exception) {
    case this->node.ku8MBSuccess:
        exceptionDescription = "";
        break;
    case this->node.ku8MBIllegalFunction:
        exceptionDescription = "Illegal function";
        break;
    case this->node.ku8MBIllegalDataAddress:
        exceptionDescription = "Illegal address";
        break;
    case this->node.ku8MBIllegalDataValue:
        exceptionDescription = "Illegal data";
        break;
    case this->node.ku8MBSlaveDeviceFailure:
        exceptionDescription = "Device failure";
        break;
    case this->node.ku8MBInvalidSlaveID:
        exceptionDescription = "Invalid ID";
        break;
    case this->node.ku8MBInvalidFunction:
        exceptionDescription = "Invalid function";
        break;
    case this->node.ku8MBResponseTimedOut:
        exceptionDescription = "Response timeout";
        break;
    case this->node.ku8MBInvalidCRC:
        exceptionDescription = "Invalid CRC";
        break;
    default:
        exceptionDescription = "Exception occurred";
        break;
    }
    return exceptionDescription;
}

String SolarModbus::exceptionHandler(uint8_t result, String mode, String address) {
    if (result != 0) {
        return (mode + " (" + address + "): " + this->exceptionDescription(result));
    }
    return "";
}

/* Modbus function 0x10 */
uint8_t SolarModbus::writeMultipleRegisters(uint16_t *data, uint16_t address, uint8_t nReg) {
    uint8_t result = 255;
    int iter = 0;
    while(result != 0 and iter < nIter) {
        for(int i = 0; i < sizeof(data)/sizeof(uint16_t); i++) {
            this->node.setTransmitBuffer(i, data[i]);
        }
        result = this->node.writeMultipleRegisters(address, nReg);
        iter ++;
    }
    return result;
}

/* Modbus function 0x05 */
uint8_t SolarModbus::writeSingleCoil(uint16_t data, uint16_t address) {
    uint8_t result = 255;
    int iter = 0;
    while (result != 0 and iter < nIter) {
        result = this->node.writeSingleCoil(address, data);
        iter++;
    }
    return result;
}

/* Modbus function 0x01 */
uint8_t SolarModbus::readCoils(uint16_t address, void *variable) {
    uint8_t result = 255;
    int iter = 0;
    while (result != 0 and iter < nIter) {
        result = this->node.readCoils(address, 1);
        iter++;
    }
    if (result == this->node.ku8MBSuccess) {
        uint16_t value = this->node.getResponseBuffer(0);
        memcpy(variable, &value, sizeof(variable));
    }
    return result;
}

/* Modbus function 0x03 */
uint8_t SolarModbus::readHoldingRegisters(uint16_t address, void *variable, uint8_t nReg) {
    uint8_t result = 255;
    int iter = 0;
    while (result != 0 and iter < nIter) {
        result = this->node.readHoldingRegisters(address, nReg);
        iter++;
    }
    if (result == this->node.ku8MBSuccess) {
        uint16_t value = this->node.getResponseBuffer(0);
        memcpy(variable, &value, sizeof(variable));
    }
    return result;
}

/* Modbus function 0x04 */
uint8_t SolarModbus::readInputRegisters(uint16_t address, void *variable, uint8_t nReg, bool oneHundredTimes) {
    uint8_t result = 255;
    int iter = 0;
    while (result != 0 and iter < nIter) {
        result = this->node.readInputRegisters(address, nReg);
        iter++;
    }
    if (result == this->node.ku8MBSuccess) {
        uint64_t value = 0x00;
        for (int i = 0; i < nReg; i++) {
            value = value | node.getResponseBuffer(i) << 16;
        }
        // float val = (oneHundredTimes) ? value / 100.0f : (float)value;
        memcpy(variable, &value, sizeof(variable));
    }
    return result;
}

/* Modbus function 0x04 */
uint8_t SolarModbus::readSingleInputRegisters(uint16_t address, void *variable, bool oneHundredTimes) {
    uint8_t result = 255;
    int iter = 0;
    while (result != 0 and iter < nIter) {
        result = this->node.readInputRegisters(address, 1);
        iter++;
    }
    if (result == this->node.ku8MBSuccess) {
        float value = (oneHundredTimes) ? node.getResponseBuffer(0)/100.0f : node.getResponseBuffer(0);
        memcpy(variable, &value, sizeof(variable));
    }
    return result;
}

void SolarModbus::setPin() {
    digitalWrite(PIN_RE_DE, HIGH);
}

void SolarModbus::resetPin() {
    digitalWrite(PIN_RE_DE, LOW);
}