#include "mqttClient.hpp"
#include "weatherJson.hpp"
#include <iostream>
#include "database.hpp"

MqttClient::MqttClient(Database& database)
    : database_(database)
{
    connect("localhost", 1883, 60);
}

void MqttClient::on_connect(int rc)
{
    if (rc == 0)
    {
        std::cout << "MQTT verbunden\n";

        subscribe(nullptr, "weather/live");
        subscribe(nullptr, "weather/history/data");

        requestSyncInfo();
    }
    else
    {
        std::cerr << "MQTT Verbindung fehlgeschlagen: "
                  << rc << '\n';
    }
}

void MqttClient::on_message(
    const struct mosquitto_message* message)
{
    // ========================================
    // MQTT-Nachricht empfangen
    // ========================================

    std::string topic(message->topic);

    std::string jsonMessage(
        static_cast<char*>(message->payload),
        message->payloadlen
    );

    std::cout
        << "MQTT Nachricht auf Topic: "
        << topic
        << '\n';


    // ========================================
    // JSON parsen
    // ========================================

    json jsonData;

    try
    {
        jsonData = json::parse(jsonMessage);
    }
    catch (const json::parse_error& e)
    {
        std::cerr
            << "JSON Fehler: "
            << e.what()
            << '\n';

        return;
    }


    // ========================================
    // weather/live
    // ========================================

    if (topic == "weather/live")
    {
        if (!validateWeatherJson(jsonData))
        {
            return;
        }

        WeatherData weather =
            weatherDataFromJson(jsonData);

        weather.print();

        if (!database_.insertWeatherData(weather))
        {
            std::cerr
                << "Wetterdaten konnten nicht gespeichert werden.\n";
        }

        return;
    }


    // ========================================
    // weather/history/data
    // ========================================

    if (topic == "weather/history/data")
    {
        if (!jsonData.contains("type"))
        {
            std::cerr
                << "History JSON: Feld 'type' fehlt\n";

            return;
        }

       
        // ------------------------------------
        // Antwort: History
        // ------------------------------------

        if (jsonData["type"] == "history")
        {
            std::cout
                << "History-Daten empfangen\n";
            
            int chunk = jsonData["chunk"];
            bool last = jsonData["last"];
            json history = jsonData["history"];

            std::cout
                << "Chunk: " << chunk << '\n';

            std::cout
                << "Datenpunkte: "
                << history.size() << '\n';

            std::cout
                << "Last: " << last << '\n';

            for (const auto& item : history)
            {
                WeatherData weather = 
                    weatherDataFromJson(item);

                std::cout
                    << "History: "
                    << weather.timestamp()
                    << " T=" << weather.temperature()
                    << " H=" << weather.humidity()
                    << " P=" << weather.pressure()
                    << '\n';
                
                bool success = database_.insertWeatherData(weather);

                std::cout
                    << "Rueckgabewert insertWeatherData: "
                    << success
                    << '\n';

                if (!success)
                {
                    std::cerr
                        << "History-Daten konnten nicht gespeichert werden.\n";
                }
            }

            if(last)
            {
                if(historyCompleteCallback_)
                {
                    historyCompleteCallback_();
                }
            }

            return;
        }


        // ------------------------------------
        // Antwort: Sync-Info
        // ------------------------------------

        if (jsonData["type"] == "sync_info")
        {
            if (!jsonData.contains("first") ||
                !jsonData.contains("last"))
            {
                std::cerr
                    << "Sync-Info JSON: Felder fehlen\n";

                return;
            }

            sdInfo_.first = jsonData["first"];
            sdInfo_.last = jsonData["last"];

            if (syncInfoCallback_)
            {
                syncInfoCallback_();
            }

            return;
        }


        std::cerr
            << "Unbekannter History-Typ\n";

        return;
    }
}


void MqttClient::requestSyncInfo()
{
    json request;

    request["type"] = "sync_info";

    std::string requestStr = request.dump();

    publish(
        nullptr,
        "weather/history/request",
        requestStr.size(),
        requestStr.c_str(),
        0,
        false
    );

    std::cout << "Anfrage nach SD-Sync-Info gesendet\n";

}