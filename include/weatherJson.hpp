#include <nlohmann/json.hpp>
#include "weather.hpp"
#include <vector>
#include "timeInterval.hpp"

using json = nlohmann::json;

bool validateWeatherJson(const json& data);

WeatherData weatherDataFromJson(const json& data);

std::vector<WeatherData> weatherDataFromJsonArray(
    const json& data
);

SyncInfo syncInfoFromJson(const json& data);

