#include "database.hpp"
#include "synchronizer.hpp"
#include "weather.hpp"
#include "timeInterval.hpp"

#include <iostream>
#include <vector>

int main()
{
    Database database("test_weather.db");

    TimeInterval interval{
        "2026-09-21 10:00",
        "2026-09-21 10:10"
    };

    std::vector<WeatherData> data;

    data.emplace_back(
        "2026-09-21 10:00:28",
        14.3,
        71.44,
        1025.82
    );

    data.emplace_back(
        "2026-09-21 10:01:28",
        14.3,
        71.95,
        1025.80
    );

    data.emplace_back(
        "2026-09-21 10:02:28",
        14.32,
        71.85,
        1025.78
    );

    data.emplace_back(
        "2026-09-21 10:03:28",
        14.35,
        71.53,
        1025.77
    );

    data.emplace_back(
        "2026-09-21 10:04:28",
        14.4,
        71.44,
        1025.79
    );

    // 10:05 fehlt absichtlich!

    data.emplace_back(
        "2026-09-21 10:06:28",
        14.42,
        71.71,
        1025.81
    );

    data.emplace_back(
        "2026-09-21 10:07:28",
        14.5,
        71.48,
        1025.81
    );

    data.emplace_back(
        "2026-09-21 10:08:28",
        14.58,
        71.11,
        1025.80
    );

    data.emplace_back(
        "2026-09-21 10:09:28",
        14.6,
        70.83,
        1025.74
    );

    data.emplace_back(
        "2026-09-21 10:10:27",
        14.67,
        70.85,
        1025.74
    );

    // Hier kommt gleich der Aufruf von processHistoryData()
}