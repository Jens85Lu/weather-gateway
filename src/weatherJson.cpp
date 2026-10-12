#include "weatherJson.hpp"
#include <iostream>

bool validateWeatherJson(const json& data)
{
    if (!data.contains("timestamp") ||
        !data.contains("temperature") ||
        !data.contains("humidity") ||
        !data.contains("pressure"))
    {
        std::cerr << "JSON Fehler: Feld fehlt\n";
        return false;
    }

    if (!data["timestamp"].is_string() ||
        !data["temperature"].is_number() ||
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
        data["temperature"],
        data["humidity"],
        data["pressure"]
    );

    return weather;

}


std::vector<WeatherData> weatherDataFromJsonArray(
    const json& data
)
{
    std::vector<WeatherData> result;

    if (!data.is_array())
    {
        std::cerr << "JSON Fehler: Array erwartet\n";
        return result;
    }

    for (const auto& entry : data)
    {
        if (!validateWeatherJson(entry))
        {
            continue;
        }

        result.push_back(
            weatherDataFromJson(entry)
        );
    }

    return result;
}

SyncInfo syncInfoFromJson(const json& data)
{
    
    SyncInfo syncInfo;
    syncInfo.first = data["first"];
    syncInfo.last = data["last"];

    return syncInfo;
}
