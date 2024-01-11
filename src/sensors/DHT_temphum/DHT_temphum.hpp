#include "header.hpp"
#include "setup.hpp"

class DHTsensor
{
    DHT_Unified dhtu;

public:
    DHTsensor();
    float readTemperature();
    float readHumidity();
};