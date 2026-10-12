#include "httpClient.hpp"

#include <httplib.h>

#include <stdexcept>

HttpClient::HttpClient(
    const std::string& esp32Address
)
    : esp32Address_(esp32Address),
      client_(esp32Address)
{
    client_.set_read_timeout(30, 0);
}

std::string HttpClient::test()
{
    auto result = client_.Get("/api/test");

    return result->body;
}

std::string HttpClient::getSyncInfo()
{
    auto result = client_.Get("/api/sync-info");

    if (!result)
    {
        throw std::runtime_error(
            "HTTP-Verbindung zum ESP32 fehlgeschlagen"
        );
    }

    if (result->status != 200)
    {
        throw std::runtime_error(
            "ESP32 antwortete mit HTTP-Status "
            + std::to_string(result->status)
        );
    }

    return result->body;
}


std::string HttpClient::getHistory(
    const TimeInterval& interval
)
{
    std::string path =
    "/api/history?from="
    + interval.from
    + "&to="
    + interval.to;
    
    std::cout << "GET: " << path << '\n';

    auto result = client_.Get(path);

    if (!result)
    {
        std::cerr
            << "HTTP-Fehler: "
            << httplib::to_string(result.error())
            << '\n';

        throw std::runtime_error(
            "HTTP-Verbindung zum ESP32 fehlgeschlagen"
        );
    }

    if (result->status != 200)
    {
        throw std::runtime_error(
            "ESP32 antwortete mit HTTP-Status "
            + std::to_string(result->status)
        );
    }

    return result->body;
}