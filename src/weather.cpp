#include "weather.hpp"

#include <iostream>

WeatherData::WeatherData(std::string timestamp,
    double temperature,
    double humidity,
    double pressure)
    : timestamp_(timestamp),
      temperature_(temperature),
      humidity_(humidity),
      pressure_(pressure)
{
}

void WeatherData::print() const
{
    std::cout << "Timestamp: " << timestamp_ << '\n';
    std::cout << "Temperature: " << temperature_ << " °C\n";
    std::cout << "Humidity: " << humidity_ << " %\n";
    std::cout << "Pressure: " << pressure_ << " hPa\n";
}

std::string WeatherData::timestamp() const
{
    return timestamp_;
}

double WeatherData::temperature() const
{
    return temperature_;
}

double WeatherData::humidity() const
{
    return humidity_;
}

double WeatherData::pressure() const
{
    return pressure_;
}