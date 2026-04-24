#pragma once

#include <iostream>

#include "Logger.hpp"
#include "curl/curl.h"


static size_t CURLWriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

class HTTPRequest {
public:

    static std::string Get(std::string& url, CURLcode& res) {
        std::string response;

        CURL* curl = curl_easy_init();
        if (!curl) {
            Logger::logf(PREFIX_ERROR, "Failed to init curl\n");
            return "";
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CURLWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            Logger::logf(PREFIX_ERROR, "CURL error: %s\n", curl_easy_strerror(res));
            return "";
        }

        return response;
    }

    static void Post(std::string& url, std::string fields, CURLcode& res) {
        std::string response;

        CURL* curl = curl_easy_init();
        if (!curl) {
            Logger::logf(PREFIX_ERROR, "Failed to init curl\n");
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CURLWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            Logger::logf(PREFIX_ERROR, "CURL (post) error: %s\n", curl_easy_strerror(res));
            return;
        }
    }
};