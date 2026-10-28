
cmake_minimum_required(VERSION 3.16)

project(weather-gateway LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)

find_library(CPP_HTTPLIB_LIBRARY
    NAMES cpp-httplib
)

# Gemeinsamer Anwendungscode
add_library(weather_gateway_core STATIC

    src/weather.cpp
    src/weatherJson.cpp

    src/mqttClient.cpp
    src/httpClient.cpp

    src/database.cpp
    src/synchronizer.cpp
)

target_include_directories(weather_gateway_core
    PUBLIC
        include
)

target_link_libraries(weather_gateway_core
    PUBLIC
        mosquittopp
        mosquitto
        SQLite::SQLite3
        ${CPP_HTTPLIB_LIBRARY}
)

# Eigentliches Programm
add_executable(weather-gateway
    src/main.cpp
)

target_link_libraries(weather-gateway
    PRIVATE
        weather_gateway_core
)

# Tests
enable_testing()

add_executable(test_database
    tests/test_database.cpp
)

target_include_directories(test_database
    PRIVATE
        include
)

target_link_libraries(test_database
    PRIVATE
        weather_gateway_core
)

add_test(
    NAME DatabaseTests
    COMMAND test_database
)