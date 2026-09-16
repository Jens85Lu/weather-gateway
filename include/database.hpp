#pragma once

#include <sqlite3.h>
#include "weather.hpp"
#include <string>
#include <vector>


struct SyncInfo
{
    std::string first;
    std::string last;
};

class Database
{
    public:
        Database(const char* filename);
        ~Database();

        bool insertWeatherData(const WeatherData& weather);

        bool isMinuteChecked(const std::string& minute);
        bool markMinuteChecked(const std::string& minute);

        SyncInfo getSyncInfo();

        std::vector<std::string> getTimestamps(
            const std::string& first,
            const std::string& last
        );


    private:

        sqlite3* db_;

        bool createTables();
};


