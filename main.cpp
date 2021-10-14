#include <iostream>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <unistd.h>
#include <curl/curl.h>
#include "data.h"
#include "567_chrono.h"
#include "567_numeric.h"
#include "567_signal.h"
#include "567_threadpool.h"

const std::string token = "62b11c372bbf05ffeda21dc10bd51bc2";
const std::string dig = "http://47.104.220.230/dig";
const std::string formula = "http://47.104.220.230/formula";

// 一次固定处理的原始数据条数
const int handleSize = 256;

std::mutex settleMutex;

int totalCount = 0;
int totalLines = 0;

size_t receive_data(void *contents, size_t size, size_t nmemb, void *stream){
    std::string  *str = (std::string*)stream;
    (*str).append((char*)contents, size * nmemb);
    return size * nmemb;
}

CURLcode PostDig(const DigData &data, std::string &response){
    std::stringstream ss;
    ajson::save_to(ss, data);
    CURLcode ret;

    CURL *pCURL = curl_easy_init();
    struct curl_slist* headers = NULL;
    if (pCURL == NULL){
        return CURLE_FAILED_INIT;
    }

    ret = curl_easy_setopt(pCURL, CURLOPT_URL, dig.c_str());
    ret = curl_easy_setopt(pCURL, CURLOPT_POST, 1L);

    headers = curl_slist_append(headers, "application/json");
    ret = curl_easy_setopt(pCURL, CURLOPT_HTTPHEADER, headers);

    ret = curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, 300);

    ret = curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, receive_data);

    ret = curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, (void*)&response);

    ret = curl_easy_perform(pCURL);
    curl_easy_cleanup(pCURL);
    return ret;
}

void HandleSourceData(std::vector<std::string> *lines){
    // 一次处理过程能得到的有效数据应该不会
    std::vector<RealData*> *datas = new std::vector<RealData*>();
    int size = lines->size();
    for (int i = 0; i < size; ++i){
        auto temp = (*lines)[i];
        SourceData src;
        ajson::load_from_buff(src, temp.c_str(), temp.size());
        __int128_t locationNumber = _567::parseFromCString(src.locationid.c_str(), src.locationid.size());
        __int128_t magicNumber = _567::parseFromCString(src.magic.c_str(), src.magic.size());

        if (locationNumber > magicNumber){
            if (locationNumber + (-1024) == magicNumber){
                auto&& data = new RealData(src.locationid, magicNumber);
                datas->push_back(data);
                continue;
            }

            if ((locationNumber & 1023) == magicNumber){
                auto&& data = new RealData(src.locationid, magicNumber);
                datas->push_back(data);
                continue;
            }
        }
        else{
            if (locationNumber + 1024 == magicNumber){
                auto&& data = new RealData(src.locationid, magicNumber);
                datas->push_back(data);
                continue;
            }

            if ((locationNumber << 10) == magicNumber){
                auto&& data = new RealData(src.locationid, magicNumber);
                datas->push_back(data);
                continue;
            }
        }
    }

    {
        std::scoped_lock<std::mutex> g(settleMutex);
        totalCount += datas->size();
    }
    ///////
//    totalCount += datas->size();
//    if (datas->size() > 0){
//        std::cout << "find " << totalCount << " datas in " << totalLines << " lines."<< std::endl;
//    }
    for (int i = 0; i < datas->size(); ++i){
        delete (*datas)[i];
    }
    delete datas;
    delete lines;
}

int main(int argc, char const *argv[]){
    // 禁掉 SIGPIPE 信号避免因为连接关闭出错
    _567::IgnoreSignal();
    auto&& pool = std::make_shared<_567::ThreadPool<void>>(4);

    auto currentMS = _567::NowMicroseconds();

    std::ifstream ifstream("./Treasure_0.data");
    if (!ifstream.is_open()){
        std::cout << "file open failed." << std::endl;
        return -1;
    }
    std::string temp;
    std::vector<std::string> *lines = new std::vector<std::string>();
    int count = 0;
    while (getline(ifstream, temp)){
        lines->push_back(temp);
        ++count;
        ++totalLines;
        if (count >= handleSize){
            //HandleSourceData(lines);
            pool->Add([lines](){
                HandleSourceData(lines);
            });
            lines = new std::vector<std::string>();
            count = 0;
        }
    }
    if (lines->size() > 0){
        //HandleSourceData(lines);
        pool->Add([lines](){
            HandleSourceData(lines);
        });
    }
    ifstream.close();

    while (pool->JobsCount() > 0){
        usleep(2000);           // 休眠2000微秒(2毫秒)
    }

    std::stringstream ss;
    DigData data("fdsafadsfasdf", token);
//    ajson::save_to(ss, data);
//    std::cout << "json: " << ss.str() << std::endl;
    std::string response;
    auto ret = PostDig(data, response);
    std::cout << "response: " << response << std::endl;

    std::cout << "find " << totalCount << " datas in " << totalLines << " lines." << std::endl;

    auto afterMS = _567::NowMicroseconds();
    auto diff = afterMS - currentMS;
    std::cout << "total cost " << diff << "ms" << std::endl;
}