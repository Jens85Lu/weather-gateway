#include "synchronizer.hpp"

#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include "weatherJson.hpp"
#include <set>

Synchronizer::Synchronizer(Database& database, MqttClient& mqttClient)
    : database_(database),
      mqttClient_(mqttClient)
{
}

void Synchronizer::onSyncInfoAvailable()
{
    std::cout
        << "Synchronizer: Sync-Info ist angekommen.\n";

    SyncInfo sqliteInfo =
        database_.getSyncInfo();

    SyncInfo sdInfo =
        mqttClient_.getSyncInfo();

    std::cout << "SQLite:\n";
    std::cout << "First: " << sqliteInfo.first << '\n';
    std::cout << "Last:  " << sqliteInfo.last << '\n';

    std::cout << "SD:\n";
    std::cout << "First: " << sdInfo.first << '\n';
    std::cout << "Last:  " << sdInfo.last << '\n';


    std::vector<std::string> missing =
        getMissingMinutes(sqliteInfo);
    
    // Aus den fehlenden Minuten Intervalle bilden
    intervals_ = createIntervals(missing);

    currentInterval_ = 0;

    // Intervalle ausgeben

    std::cout <<"Anzahl der Intervalle: "
              << intervals_.size()
              << '\n';

    std::cout << "Intervalle:\n";

    for (const auto& interval : intervals_)
    {
        std::cout
            << interval.from
            << " - "
            << interval.to
            << '\n';
    }
    requestHistory(intervals_);
}

void Synchronizer::onHistoryComplete()
{
    const TimeInterval& interval =
        intervals_[currentInterval_];

    std::vector<std::string> timestamps =
        database_.getTimestamps(
            interval.from + ":00",
            interval.to + ":59"
        );

    std::set<std::string> foundMinutes;

    for (const std::string& timestamp : timestamps)
    {
        foundMinutes.insert(
            timestamp.substr(0, 16)
        );
    }

    std::string current = interval.from;

    while (current <= interval.to)
    {
        if (foundMinutes.find(current)
            == foundMinutes.end())
        {
            std::cout
                << "Minute auf SD nicht vorhanden, "
                "markiere als geprüft: "
                << current
                << '\n';
                
            if (!database_.markMinuteChecked(current))
            {
                std::cerr
                    << "Minute konnte nicht als geprüft "
                       "markiert werden: "
                    << current
                    << '\n';
            }
        }

        std::string next =
            addMinute(current + ":00");

        if (next.empty())
        {
            break;
        }

        current = next.substr(0, 16);
    }

    ++currentInterval_;

    requestHistory(intervals_);
}

std::string Synchronizer::addMinute(
    const std::string &timestamp)
    {
        std::tm time{};

        std::istringstream input(timestamp);
        input >> std::get_time(&time, "%Y-%m-%d %H:%M:%S");

        if (input.fail())
        {
            return "";
        }

        time.tm_min += 1;
        time.tm_isdst = -1;

        if (std::mktime(&time) == -1)
        {
            return "";
        }

        std::ostringstream output;

        output << std::put_time(&time, "%Y-%m-%d %H:%M:%S");

        return output.str();
    }

    std::vector<std::string> Synchronizer::getMissingMinutes(
    const SyncInfo& sqliteInfo)
    {
        std::vector<std::string> missing;

        std::vector<std::string> timestamps =
            database_.getTimestamps(
                sqliteInfo.first,
                sqliteInfo.last
            );

        std::set<std::string> sqliteMinutes;

        for (const std::string& timestamp : timestamps)
        {
            sqliteMinutes.insert(
                timestamp.substr(0, 16)
            );
        }

        std::string current =
            sqliteInfo.first.substr(0, 16);

        std::string last =
            sqliteInfo.last.substr(0, 16);

        while (current <= last)
        {
            if (sqliteMinutes.find(current)
                == sqliteMinutes.end()
                && !database_.isMinuteChecked(current))
            {
                missing.push_back(current);
            }

            std::string next =
                addMinute(current + ":00");

            if (next.empty())
            {
                break;
            }

            current = next.substr(0, 16);
        }

        SyncInfo sdInfo =
        mqttClient_.getSyncInfo();


        // Fehlende Minuten nach SQLite.last

        current = addMinute(sqliteInfo.last + ":00").substr(0, 16);

        last = sdInfo.last.substr(0, 16);

        while (current <= last)
        {
            if (!database_.isMinuteChecked(current))
            {
                missing.push_back(current);
            }
    
            std::string next =
                addMinute(current + ":00");

            if (next.empty())
            {
                break;
            }

            current = next.substr(0, 16);
        }


        // Fehlende Minuten vor SQLite.first
        current =
            sdInfo.first.substr(0, 16);

        last =
            sqliteInfo.first.substr(0, 16);

        while (current < last)
        {
            if (!database_.isMinuteChecked(current))
            {
                missing.push_back(current);
            }

            std::string next =
                addMinute(current + ":00");

            if (next.empty())
            {
                break;
            }

            current = next.substr(0, 16);
        }

        std::sort(missing.begin(), missing.end());



        return missing;
    }

    std::vector<TimeInterval> Synchronizer::createIntervals(
    const std::vector<std::string>& missing)
    {
        std::vector<TimeInterval> intervals;

        if (missing.empty())
        {
            return intervals;
        }

        std::string start = missing[0];
        std::string previous = missing[0];

        for (std::size_t i = 1; i < missing.size(); ++i)
        {
            std::string expected =
                addMinute(previous + ":00");

            if (missing[i] == expected.substr(0, 16))
            {
                previous = missing[i];
            }
            else
            {
                intervals.push_back(
                    {start, previous}
                );

                start = missing[i];
                previous = missing[i];
            }
        }

        intervals.push_back(
            {start, previous}
        );

        return intervals;
    }

void Synchronizer::requestHistory(
    const std::vector<TimeInterval>& intervals)
{
    if (currentInterval_ >= intervals.size())
    {
        std::cout
            << "Alle Intervalle synchronisiert.\n";

        return;
    }

    const TimeInterval& interval =
        intervals[currentInterval_];

    json request;
    request["type"] = "history";

    json item;
    item["from"] = interval.from;
    item["to"] = interval.to;

    request["intervals"].push_back(item);

    std::string payload = request.dump();

    std::cout
        << "Fordere Intervall "
        << currentInterval_ + 1
        << " / "
        << intervals.size()
        << " an:\n"
        << payload
        << '\n';

    mqttClient_.publish(
        nullptr,
        "weather/history/request",
        payload.size(),
        payload.c_str(),
        0,
        false
    );
}