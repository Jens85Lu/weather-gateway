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
find_package(CURL REQUIRED)

message(STATUS "CURL_LIBRARIES = ${CURL_LIBRARIES}")
message(STATUS "CURL_INCLUDE_DIRS = ${CURL_INCLUDE_DIRS}")

target_include_directories(weather-gateway
    PRIVATE
        include
        ${CURL_INCLUDE_DIRS}
)

target_link_libraries(weather-gateway
    mosquittopp
    mosquitto
    SQLite::SQLite3
    ${CURL_LIBRARIES}
)