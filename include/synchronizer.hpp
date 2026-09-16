#pragma once

#include "database.hpp"
#include "mqttClient.hpp"
#include <string>
#include <vector>

struct TimeInterval
{
    std::string from;
    std::string to;
};


class Synchronizer
{
public:
    Synchronizer(Database& database, MqttClient& mqttClient);
    
    void onSyncInfoAvailable();

    void onHistoryComplete();

private:
    Database& database_;
    MqttClient& mqttClient_;

    std::string addMinute(
        const std::string& timestamp
    );

    std::vector<std::string> getMissingMinutes(
        const SyncInfo& sqliteInfo
    );

    std::vector<TimeInterval> createIntervals(
        const std::vector<std::string>& missing
    );

    void requestHistory(
        const std::vector<TimeInterval>& intervals
    );

    std::vector<TimeInterval> intervals_;
    
    std::size_t currentInterval_ = 0;
};