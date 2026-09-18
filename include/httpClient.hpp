#pragma once

#include <string>

class HttpClient
{
public:
    explicit HttpClient(const std::string& esp32Address);

    std::string getHistory(
        const std::string& from,
        const std::string& to
    );

private:
    std::string esp32Address_;
};