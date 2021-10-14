#include <iostream>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <unistd.h>
#include "data.h"
#include "567_chrono.h"
#include "567_numeric.h"
#include "567_signal.h"
#include "567_threadpool.h"

const std::string token = "62b11c372bbf05ffeda21dc10bd51bc2";

// 一次固定处理的原始数据条数
const int handleSize = 256;

std::mutex settleMutex;

int totalCount = 0;
int totalLines = 0;

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
    auto&& pool = std::make_shared<_567::ThreadPool<void>>(2);

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

    std::cout << "find " << totalCount << " datas in " << totalLines << " lines." << std::endl;

    auto afterMS = _567::NowMicroseconds();
    auto diff = afterMS - currentMS;
    std::cout << "total cost " << diff << "ms" << std::endl;
}