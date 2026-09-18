#include "httpClient.hpp"

#include <curl/curl.h>

#include <iostream>
#include <stdexcept>

namespace
{

size_t writeCallback(
    char* ptr,
    size_t size,
    size_t nmemb,
    void* userdata
)
{
    std::string* response =
        static_cast<std::string*>(userdata);

    response->append(ptr, size * nmemb);

    return size * nmemb;
}

}

HttpClient::HttpClient(
    const std::string& esp32Address
)
    : esp32Address_(esp32Address)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}


std::string HttpClient::getHistory(
    const std::string& from,
    const std::string& to
)
{
    CURL* curl = curl_easy_init();

    if (!curl)
    {
        throw std::runtime_error(
            "curl_easy_init fehlgeschlagen"
        );
    }

    std::string response;

    char* encodedFrom =
        curl_easy_escape(curl, from.c_str(), 0);

    char* encodedTo =
        curl_easy_escape(curl, to.c_str(), 0);

    if (!encodedFrom || !encodedTo)
    {
        curl_free(encodedFrom);
        curl_free(encodedTo);
        curl_easy_cleanup(curl);

        throw std::runtime_error(
            "URL-Encoding fehlgeschlagen"
        );
    }

    std::string url =
        esp32Address_
        + "/api/history?from="
        + encodedFrom
        + "&to="
        + encodedTo;

    curl_free(encodedFrom);
    curl_free(encodedTo);

    std::cout
        << "HTTP GET: "
        << url
        << '\n';

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        writeCallback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &response
    );

    curl_easy_setopt(
        curl,
        CURLOPT_TIMEOUT,
        10L
    );

    CURLcode result =
        curl_easy_perform(curl);

    if (result != CURLE_OK)
    {
        std::cerr
            << "HTTP Fehler: "
            << curl_easy_strerror(result)
            << '\n';
    }
    else
    {
        long httpStatus = 0;

        curl_easy_getinfo(
            curl,
            CURLINFO_RESPONSE_CODE,
            &httpStatus
        );

        std::cout
            << "HTTP Status: "
            << httpStatus
            << '\n';
    }

    curl_easy_cleanup(curl);

    return response;
}
