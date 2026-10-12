cmake_minimum_required(VERSION 3.16)

project(weather-gateway)

add_executable(weather-gateway

    src/main.cpp

    src/weather.cpp
    src/weatherJson.cpp

    src/mqttClient.cpp
    src/httpClient.cpp

    src/database.cpp
    src/synchronizer.cpp

)

target_include_directories(weather-gateway

    PRIVATE

        include

)

find_package(SQLite3 REQUIRED)

find_library(CPP_HTTPLIB_LIBRARY
    NAMES cpp-httplib
)

target_include_directories(weather-gateway
    PRIVATE
        include
)

target_link_libraries(weather-gateway
    mosquittopp
    mosquitto
    SQLite::SQLite3
    ${CPP_HTTPLIB_LIBRARY}
)