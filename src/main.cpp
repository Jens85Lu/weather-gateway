#include "weather.hpp"
#include "weatherJson.hpp"
#include "mqttClient.hpp"
#include "database.hpp"
#include "synchronizer.hpp"
#include "httpClient.hpp"

#include <iostream>

int main()
{
    // Database database("database/weather.db");

    // mosqpp::lib_init();

    // MqttClient client(database);

    // Synchronizer synchronizer(database, client);

    // // nach Empfang der SD Zeitinfo: Berechnung und Anforderung der fehlenden Minuten 
    // client.setSyncInfoCallback(
    //     [&synchronizer]() 
    //     {
    //     synchronizer.onSyncInfoAvailable();
    //     });

    // // nach dem kompletten Empfang der fehlenden Minuten eines Intervalls
    // // Anforderung des nächsten Intervalls
    // client.setHistoryCompleteCallback( // nach dem kompletten Empfang des ersten History Intervalls
    //     [&synchronizer]()
    //     {
    //         synchronizer.onHistoryComplete();
    //     });

    // client.loop_forever();

    // mosqpp::lib_cleanup(); 

    HttpClient httpClient("192.168.0.17");

    std::string response =
        httpClient.getHistory(
            "2026-09-17 10:00",
            "2026-09-17 10:10"
        );

    std::cout << response << '\n';


    return 0;
}