cmake_minimum_required(VERSION 3.16)

project(weather-gateway)

add_executable(weather-gateway
    src/main.cpp
    src/weather.cpp
    src/weatherJson.cpp
    src/mqttClient.cpp
    src/database.cpp
    src/synchronizer.cpp
)

target_include_directories(weather-gateway
    PRIVATE
        include
)

find_package(SQLite3 REQUIRED)

target_link_libraries(weather-gateway
    mosquittopp
    mosquitto
    SQLite::SQLite3)