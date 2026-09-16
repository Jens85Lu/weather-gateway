#include <nlohmann/json.hpp>
#include "weather.hpp"

using json = nlohmann::json;

bool validateWeatherJson(const json& data);
WeatherData weatherDataFromJson(const json& data);