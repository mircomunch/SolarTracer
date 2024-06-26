#include "DHT_temphum.hpp"
#include "dht11_setup.hpp"

DHTsensor::DHTsensor() : dhtu(DHTPIN, DHTTYPE)
{}

float DHTsensor::readTemperature()
{
    sensors_event_t event;
    this->dhtu.temperature().getEvent(&event);
    int temperature = event.temperature;

    // Serial.printf("Temperature: %2d °C\n", temperature);
    return temperature;
}

float DHTsensor::readHumidity()
{
    sensors_event_t event;
    this->dhtu.humidity().getEvent(&event);
    float humidity = event.relative_humidity;

    // Serial.printf("Humidity: %.1d °C\n", humidity);
    return humidity;
}