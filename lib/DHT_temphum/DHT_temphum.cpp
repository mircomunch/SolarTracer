#include <DHT_temphum.hpp>

DHTsensor::DHTsensor() : dhtu(13, DHT11)
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