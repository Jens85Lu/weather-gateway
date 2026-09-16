#include "weatherJson.hpp"
#include <iostream>

bool validateWeatherJson(const json& data)
{
    if (!data.contains("timestamp") ||
        !data.contains("temp") ||
        !data.contains("humidity") ||
        !data.contains("pressure"))
    {
        std::cerr << "JSON Fehler: Feld fehlt\n";
        return false;
    }

    if (!data["timestamp"].is_string() ||
        !data["temp"].is_number() ||
        !data["humidity"].is_number() ||
        !data["pressure"].is_number())
    {
        std::cerr << "JSON Fehler: Falscher Datentyp\n";
        return false;
    }
    
    return true;
}

WeatherData weatherDataFromJson(const json& data)
{
    WeatherData weather(
        data["timestamp"],
        data["temp"],
        data["humidity"],
        data["pressure"]
    );

    return weather;

}

