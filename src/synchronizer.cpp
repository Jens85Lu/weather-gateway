#include "synchronizer.hpp"

#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include "weatherJson.hpp"
#include <set>

Synchronizer::Synchronizer(Database& database, MqttClient& mqttClient, HttpClient& httpClient)
    : database_(database),
      mqttClient_(mqttClient),
      httpClient_(httpClient)
{
}


void Synchronizer::startSync()
{
    SyncInfo sqliteInfo = database_.getSyncInfo();

    std::string response = httpClient_.getSyncInfo();
    json data = json::parse(response);
    SyncInfo sdInfo = syncInfoFromJson(data);

    std::vector<std::string> missingMinutes =
        getMissingMinutes(sqliteInfo, sdInfo);

    std::vector<TimeInterval> intervals =
        createIntervals(missingMinutes);

    intervals = splitIntoDays(intervals);
    
    synchronizeIntervals(intervals);
}


void Synchronizer::synchronizeIntervals(
    const std::vector<TimeInterval>& intervals)
{
    for (const TimeInterval& interval : intervals)
    {
        std::string response = httpClient_.getHistory(interval);

        std::cout << "Antwort:\n";

        std::cout << response << '\n';

        json data = json::parse(response);  

        std::cout << "JSON erfolgreich geparst.\n";

        std::vector<WeatherData> weatherData =
            weatherDataFromJsonArray(data);

        processHistoryInterval(
            interval,
            weatherData
        );
    }
}

void Synchronizer::processHistoryInterval(
    const TimeInterval& interval,
    const std::vector<WeatherData>& weatherData)
{
    std::set<std::string> foundMinutes;

    for (const WeatherData& weather : weatherData)
    {
        if (!database_.insertWeatherData(weather))
        {
            std::cerr
                << "Wetterdaten konnten nicht gespeichert werden.\n";
        }

        foundMinutes.insert(
            weather.timestamp().substr(0, 16)
        );
    }

    std::string current = interval.from;

    while (current <= interval.to)
    {
        if (foundMinutes.find(current)
            == foundMinutes.end())  // nicht gefunden
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
        getMissingMinutes(sqliteInfo, sdInfo);
    
    // Aus den fehlenden Minuten Intervalle bilden
    intervals_ = createIntervals(missing);

    intervals_ = splitIntoDays(intervals_);
    
    // Intervalle ausgeben

    std::cout <<"Anzahl der Intervalle: "
              << intervals_.size()
              << '\n';

    if (intervals_.empty())
    {
        std::cout
            << "Keine fehlenden Daten. "
            "Synchronisation abgeschlossen.\n";

        return;
    }
    

    requestHistoryMQTT(intervals_[currentInterval_]);
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

    if(currentInterval_ < intervals_.size())
    {
    requestHistoryMQTT(intervals_[currentInterval_]);
    }
    else
    {
        std::cout << "Alle Intervalle synchronisiert.\n";
    }
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
    const SyncInfo& sqliteInfo,
    const SyncInfo& sdInfo
)
{
    std::vector<std::string> missing;

    if (sqliteInfo.first.empty())
    {
        // SQLite ist leer:
        // Der komplette Zeitraum der SD-Karte fehlt.

        std::string current =
            sdInfo.first.substr(0, 16);

        std::string last =
            sdInfo.last.substr(0, 16);

        while (current <= last)
        {
            missing.push_back(current);

            std::string next =
                addMinute(current + ":00");

            if (next.empty())
            {
                break;
            }

            current = next.substr(0, 16);
        }

        return missing;
    }

    std::vector<std::string> timestamps =
        database_.getTimestamps(
            sqliteInfo.first,
            sqliteInfo.last
        );
    // Put sqlite timestamps in a set

    std::set<std::string> sqliteMinutes;

    for (const std::string& timestamp : timestamps)
    {
        sqliteMinutes.insert(
            timestamp.substr(0, 16)
        );
    }
    // Go through consecutive sqlite minutes between first and last
    std::string current =
        sqliteInfo.first.substr(0, 16);

    std::string last =
        sqliteInfo.last.substr(0, 16);

    while (current <= last)
    {
        if (sqliteMinutes.find(current) // current minute missing
            == sqliteMinutes.end()
            && !database_.isMinuteChecked(current)) // & not already checked
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

    // Get missing minutes after SQLite.last

    std::string next =
        addMinute(sqliteInfo.last + ":00");

    if (next.empty())
    {
        return missing;
    }

    current = next.substr(0, 16);

    last = sdInfo.last.substr(0, 16);

    while (current <= last)
    {
        if (!database_.isMinuteChecked(current)) // if not already checked
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


    // Get missing minutes before SQLite.first

    current =
        sdInfo.first.substr(0, 16);

    last =
        sqliteInfo.first.substr(0, 16);

    while (current < last)
    {
        if (!database_.isMinuteChecked(current)) // if not already checked
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

std::vector<TimeInterval> Synchronizer::splitIntoDays(
    const std::vector<TimeInterval>& intervals
)
{
    std::vector<TimeInterval> result;

    for (const TimeInterval& interval : intervals)
    {
        std::string current = interval.from;

        while (true)
        {
            std::string currentDate =
                current.substr(0, 10);

            std::string endOfDay =
                currentDate + " 23:59";

            if (currentDate == interval.to.substr(0, 10))
            {
                result.push_back({
                    current,
                    interval.to
                });

                break;
            }
            else
            {
                result.push_back({
                    current,
                    endOfDay
                });

                std::string next =
                    addMinute(endOfDay + ":00");

                current =
                    next.substr(0, 16);
            }
        }
    }

    return result;
}

void Synchronizer::requestHistoryMQTT(
    const TimeInterval& interval)
{
    json request;
    request["type"] = "history";

    json item;
    item["from"] = interval.from;
    item["to"] = interval.to;

    request["interval"]= item;

    std::string payload = request.dump();

    std::cout
        << "Fordere Tagesintervall "
        << currentInterval_ + 1
        << " / "
        << intervals_.size()
        << " an:\n"
        << interval.from
        << " - "
        << interval.to
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