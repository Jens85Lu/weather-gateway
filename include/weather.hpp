#pragma once

#include <string>

class WeatherData
{
public:
    WeatherData(std::string timestamp, double temperature,
        double humidity,
        double pressure);
    
        void print() const;
        std::string timestamp() const;
        double temperature() const;
        double humidity() const;
        double pressure() const;

private:
    std::string timestamp_;
    double temperature_;
    double humidity_;
    double pressure_;
};

