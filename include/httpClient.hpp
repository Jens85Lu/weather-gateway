#pragma once

#include "timeInterval.hpp"
#include <httplib.h>
#include <string>

class HttpClient
{
public:
    explicit HttpClient(const std::string& esp32Address);

    std::string getSyncInfo();
    std::string getHistory(const TimeInterval& interval);
    std::string test();

private:
    std::string esp32Address_;
    httplib::Client client_;
};