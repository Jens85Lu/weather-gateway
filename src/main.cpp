#include "weather.hpp"
#include "weatherJson.hpp"
#include "mqttClient.hpp"
#include "database.hpp"
#include "synchronizer.hpp"
#include "httpClient.hpp"

#include <iostream>
#include <chrono>
#include <thread>

int main()
{
    Database database("database/weather.db");

    mosqpp::lib_init();

    MqttClient mqttClient(database);
    HttpClient httpClient("192.168.0.17");

    Synchronizer synchronizer(database, mqttClient, httpClient);

    // Nach Empfang der SD-Zeitinfo:
    // Berechnung und Anforderung der fehlenden Minuten
    mqttClient.setSyncInfoCallback(
        [&synchronizer]()
        {
            synchronizer.onSyncInfoAvailable();
        });

    // Nach dem kompletten Empfang der fehlenden Minuten eines Intervalls:
    // Anforderung des nächsten Intervalls
    mqttClient.setIntervalCompleteCallback(
        [&synchronizer]()
        {
            synchronizer.onIntervalComplete();
        });

    mqttClient.loop_forever();

    mosqpp::lib_cleanup();

    return 0;
}