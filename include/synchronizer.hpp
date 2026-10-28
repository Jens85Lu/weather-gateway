#pragma once

#include "database.hpp"
#include "mqttClient.hpp"
#include <string>
#include <vector>
#include "timeInterval.hpp"
#include "weather.hpp"
#include "httpClient.hpp"

class Synchronizer
{
public:
    Synchronizer(Database& database, MqttClient& mqttClient, HttpClient&);

    void startSync();

    void synchronizeIntervals(const std::vector<TimeInterval>& intervals);
    
    void onSyncInfoAvailable();

    void onIntervalComplete();

    bool processHistoryInterval(const std::vector<WeatherData>& weatherData);

    
private:
    Database& database_;
    MqttClient& mqttClient_;
    HttpClient& httpClient_;

    std::string plusMinute(
        const std::string& timestamp
    );

    std::string minusMinute(
    const std::string &timestamp
    );

    std::vector<std::string> getMissingMinutes(
    const SyncInfo& sdInfo, std::string& lastSync
    );

    std::vector<TimeInterval> createIntervals(
        const std::vector<std::string>& missing
    );

    std::vector<TimeInterval> splitIntoDays(
    const std::vector<TimeInterval>& intervals
    );    

    void requestHistoryMQTT(
        const TimeInterval& intervals
    );

    std::vector<TimeInterval> intervals_;
    
    std::size_t currentInterval_ = 0;
};

