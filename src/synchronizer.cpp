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
    std::cout << "HTTP sync gestartet\n";
    std::string response = httpClient_.getSyncInfo();

    json data = json::parse(response);
    SyncInfo sdInfo = syncInfoFromJson(data);
    std::string lastSync = database_.getLastSync();

    std::vector<std::string> missingMinutes =
        getMissingMinutes(sdInfo, lastSync);

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
        try
        {
            std::string response =
                httpClient_.getHistory(interval);

            std::cout << "Antwort:\n";
            std::cout << response << '\n';

            json data = json::parse(response);

            if (!data.is_array())
            {
                throw std::runtime_error(
                    "JSON-Array erwartet"
                );
            }

            std::cout << "JSON erfolgreich geparst.\n";

            std::vector<WeatherData> weatherData =
                weatherDataFromJsonArray(data);

            if (!processHistoryInterval(weatherData))
            {
                std::cerr
                    << "Intervall konnte nicht vollständig "
                    "verarbeitet werden.\n";
                return;
            }

            if (!database_.markLastSync(interval.to))
            {
                std::cerr
                    << "Last Sync konnte nicht aktualisiert werden.\n";
                return;
            }

            std::cout
                << "Intervall erfolgreich verarbeitet bis "
                << interval.to << '\n';
        }
        catch (const std::exception& e)
        {
            std::cerr
                << "Fehler beim Synchronisieren des Intervalls "
                << interval.from << " bis " << interval.to
                << ": " << e.what() << '\n';

            return;
        }
    }


    std::cout << "Alle Intervalle synchronisiert.\n";
}

bool Synchronizer::processHistoryInterval(
    const std::vector<WeatherData>& weatherData)
{

    for (const WeatherData& weather : weatherData)
    {
        if (!database_.insertWeatherData(weather))
        {
            std::cerr
                << "Wetterdaten konnten nicht gespeichert werden.\n";

            return false;
        }

    }

    return true;
    
}

void Synchronizer::onSyncInfoAvailable()
{
    currentInterval_ = 0;
    mqttClient_.historyStorageSuccessful = true;

    std::cout
        << "Synchronizer: Sync-Info ist angekommen.\n";

    SyncInfo sdInfo =
        mqttClient_.getSyncInfo();

    std::string lastSync = database_.getLastSync();

    std::cout << "Last Sync:\n";
    std::cout << lastSync << "\n";

    std::cout << "SD:\n";
    std::cout << "First: " << sdInfo.first << '\n';
    std::cout << "Last:  " << sdInfo.last << '\n';


    std::vector<std::string> missing =
        getMissingMinutes(sdInfo, lastSync);
    
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

void Synchronizer::onIntervalComplete()
{
   const TimeInterval& interval =
        intervals_[currentInterval_];

    if (!database_.markLastSync(interval.to))
    {
        std::cerr
            << "Last Sync konnte nicht aktualisiert werden.\n";
        return;
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

std::string Synchronizer::plusMinute(
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

std::string Synchronizer::minusMinute(
    const std::string &timestamp)
{
    std::tm time{};

    std::istringstream input(timestamp);
    input >> std::get_time(&time, "%Y-%m-%d %H:%M:%S");

    if (input.fail())
    {
        return "";
    }

    time.tm_min -= 1;
    time.tm_isdst = -1;

    if (std::mktime(&time) == -1)
    {
        return "";
    }

    std::ostringstream output;

    output << std::put_time(&time, "%Y-%m-%d %H:%M:%S");

    return output.str();
}

bool containsMinute(
    const std::set<std::string>& minutes,
    const std::string& current
)
{
    return minutes.find(current) != minutes.end();
}

std::vector<std::string> Synchronizer::getMissingMinutes(
    const SyncInfo& sdInfo, std::string& lastSync
)
{
    if (lastSync.empty())
    {
        lastSync = minusMinute(sdInfo.first).substr(0,16);
    }
    
    // sqlite timestampts between lastSync and sdInfo.last
    std::vector<std::string> timestamps =
        database_.getTimestamps(
            lastSync + ":00",
            sdInfo.last
        );
    
     // Put sqlite timestamps in a set

    std::set<std::string> sqliteMinutes;

    for (const std::string& timestamp : timestamps)
    {
        sqliteMinutes.insert(
            timestamp.substr(0, 16)
        );
    }

    // start with minute after lastSync
    std::string current = plusMinute(lastSync + ":00").substr(0,16);
    // end with last minute on SD
    std::string end = sdInfo.last.substr(0,16);
    // missing minutes on DB
    std::vector<std::string> missing;

    while(current <= end)
    {
        if(!containsMinute(sqliteMinutes, current))
        {
            missing.push_back(current);
        }

        current = plusMinute(current + ":00").substr(0,16);
    }

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
            plusMinute(previous + ":00");

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
                    plusMinute(endOfDay + ":00");

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