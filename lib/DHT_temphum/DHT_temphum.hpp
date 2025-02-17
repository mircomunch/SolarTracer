#include <Adafruit_Sensor.h>
#include <DHT_U.h>

class DHTsensor
{
    DHT_Unified dhtu;

public:
    DHTsensor();
    float readTemperature();
    float readHumidity();
};