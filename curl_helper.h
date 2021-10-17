#pragma once

#include <curl/curl.h>
#include "data.h"

const std::string token = "62b11c372bbf05ffeda21dc10bd51bc2";
const std::string dig = "http://47.104.220.230/dig";
const std::string formula = "http://47.104.220.230/formula";

size_t receive_data(void *contents, size_t size, size_t nmemb, void *stream){
    std::string  *str = (std::string*)stream;
    (*str).append((char*)contents, size * nmemb);
    return size * nmemb;
}

inline int PostDig(const std::string &locationId){
    DigData data(locationId, token);
    std::stringstream ss;
    ajson::save_to(ss, data);
    CURLcode ret;

    CURL *pCURL = curl_easy_init();
    struct curl_slist* headers = NULL;
    if (pCURL == NULL){
        return CURLE_FAILED_INIT;
    }

    std::string response;
    ret = curl_easy_setopt(pCURL, CURLOPT_URL, dig.c_str());
    ret = curl_easy_setopt(pCURL, CURLOPT_POST, 1L);

    headers = curl_slist_append(headers, "application/json");
    ret = curl_easy_setopt(pCURL, CURLOPT_HTTPHEADER, headers);

    ret = curl_easy_setopt(pCURL, CURLOPT_POSTFIELDS, ss.str().c_str());

    ret = curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, 1);

    ret = curl_easy_setopt(pCURL, CURLOPT_CONNECTTIMEOUT, 3);

    ret = curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, receive_data);

    ret = curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, (void*)&response);

    ret = curl_easy_perform(pCURL);
    curl_easy_cleanup(pCURL);

    if (ret == CURLE_OK){
        DigResult result;
        ajson::load_from_buff(result, response.c_str(), response.size());
        return result.errorno;
    }

    return 1;
}