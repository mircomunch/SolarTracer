#include <powerReader.hpp>

// Costruttore
PowerReader::PowerReader(uint8_t txPin, uint8_t rxPin, uint32_t baudRate, uint8_t slaveID,
                         int nIter, Stream &serial, HardwareSerial &serial_stream)
{
    this->_nIter = nIter;
    this->_txPin = txPin;
    this->_rxPin = rxPin;
    this->_slaveID = slaveID;
    this->_baudRate = baudRate;
    this->_serial = serial;
    this->_serialPort = serial_stream;
}

void PowerReader::begin() {
    this->_serialPort.begin(_baudRate, SERIAL_8N1, _rxPin, _txPin);
    this->_modbusNode.begin(_slaveID, _serial);
}

String PowerReader::exceptionDescription(uint8_t exception)
{
    String exceptionDescription = "";
    switch (exception)
    {
    case this->_modbusNode.ku8MBSuccess:
        exceptionDescription = "";
        break;
    case this->_modbusNode.ku8MBIllegalFunction:
        exceptionDescription = "Illegal function";
        break;
    case this->_modbusNode.ku8MBIllegalDataAddress:
        exceptionDescription = "Illegal address";
        break;
    case this->_modbusNode.ku8MBIllegalDataValue:
        exceptionDescription = "Illegal data";
        break;
    case this->_modbusNode.ku8MBSlaveDeviceFailure:
        exceptionDescription = "Device failure";
        break;
    case this->_modbusNode.ku8MBInvalidSlaveID:
        exceptionDescription = "Invalid ID";
        break;
    case this->_modbusNode.ku8MBInvalidFunction:
        exceptionDescription = "Invalid function";
        break;
    case this->_modbusNode.ku8MBResponseTimedOut:
        exceptionDescription = "Response timeout";
        break;
    case this->_modbusNode.ku8MBInvalidCRC:
        exceptionDescription = "Invalid CRC";
        break;
    default:
        exceptionDescription = "Exception occurred";
        break;
    }
    return exceptionDescription;
}

// Funzione generica per leggere un registro Modbus
uint8_t PowerReader::readModbusRegister(uint16_t reg, float divisor, float *variable)
{
    uint8_t result = 255;
    int iter = 0;
    while (result != 0 and iter < this->_nIter)
    {
        result = this->_modbusNode.readHoldingRegisters(reg, 2);
        iter++;
    }

    if (result == this->_modbusNode.ku8MBSuccess)
    {
        uint32_t rawData = (this->_modbusNode.getResponseBuffer(0) << 16) | this->_modbusNode.getResponseBuffer(1);
        float value = rawData / divisor;
        memcpy(variable, &value, sizeof(variable));
    }
    return result;
}

// Legge l'indirizzo di test everifica che contenga il valore aspettato (0x0194)
uint8_t PowerReader::test()
{
    uint8_t result = 255;
    int iter = 0;
    uint16_t variable;
    while (result != 0 and iter < this->_nIter)
    {
        result = this->_modbusNode.readHoldingRegisters(0x0000, 1);
        iter++;
    }

    if (result == this->_modbusNode.ku8MBSuccess)
    {
        uint16_t value = this->_modbusNode.getResponseBuffer(0);
        memcpy(&variable, &value, sizeof(variable));
    }
    if (variable != 404)
    {
        result = 255;
    }
    return result;
}

// Legge la tensione (Voltage) per il canale specificato
uint8_t PowerReader::readVoltage(int channel, float *variable)
{
    uint8_t result = 254;
    if (channel == 1)
    {
        result = readModbusRegister(0x0048, 10000.0, variable);
    }
    else if (channel == 2)
    {
        result = readModbusRegister(0x0050, 10000.0, variable);
    }
    return result;
}

// Legge la corrente (Current) per il canale specificato
uint8_t PowerReader::readCurrent(int channel, float *variable)
{
    uint8_t result = 254;
    if (channel == 1)
    {
        result = readModbusRegister(0x0049, 10000.0, variable);
    }
    else if (channel == 2)
    {
        result = readModbusRegister(0x0051, 10000.0, variable);
    }
    return result;
}

// Legge la potenza (Power) per il canale specificato
uint8_t PowerReader::readPower(int channel, float *variable)
{
    uint8_t result = 254;
    if (channel == 1)
    {
        result = readModbusRegister(0x004A, 10000.0, variable);
    }
    else if (channel == 2)
    {
        result = readModbusRegister(0x0052, 10000.0, variable);
    }
    return result;
}

// Legge l'energia (Energy) per il canale specificato
uint8_t PowerReader::readEnergy(int channel, float *variable)
{
    uint8_t result = 254;
    if (channel == 1)
    {
        result = readModbusRegister(0x004B, 10000.0, variable);
    }
    else if (channel == 2)
    {
        result = readModbusRegister(0x0053, 10000.0, variable);
    }
    return result;
}

// Legge Power factor per il canale specificato
uint8_t PowerReader::readPowerfactor(int channel, float *variable)
{
    uint8_t result = 254;
    if (channel == 1)
    {
        result = readModbusRegister(0x004C, 1000.0, variable);
    }
    else if (channel == 2)
    {
        result = readModbusRegister(0x0054, 1000.0, variable);
    }
    return result;
}

// Legge Power direction per il entrambi i canali
uint8_t PowerReader::readPowerdirection(float *variable)
{
    uint8_t result = 254;
    result = readModbusRegister(0x004E, 1.0, variable);
    return result;
}

// Legge Frequenza
uint8_t PowerReader::readFrequency(float *variable)
{
    uint8_t result = 254;
    result = readModbusRegister(0x004F, 100.0, variable);
    return result;
}
