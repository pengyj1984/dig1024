#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <unistd.h>
#include <string.h>

#include "567_chrono.h"
#include "567_numeric.h"
#include "567_signal.h"
#include "567_threadpool.h"
#include "curl_helper.h"
#include "mempool.h"

std::mutex settleMutex;

MemPool *memPool;

int totalCount = 0;
int totalLines = 0;
std::vector<RealData*> *realDatas;

void HandleSourceData(MemChunk *chunk){
    // 一次处理过程能得到的有效数据应该不会
    std::vector<RealData*> *datas = new std::vector<RealData*>();
    int size = chunk->size;
    int passCount = 0;
    std::string response;
    for (int i = 0; i < size; ++i){
        auto temp = chunk->datas[i];
        SourceData src;
        ajson::load_from_buff(src, temp.buffer, temp.size);
        __int128_t locationNumber = _567::parseFromCString(src.locationid.c_str(), src.locationid.size());
        __int128_t magicNumber = _567::parseFromCString(src.magic.c_str(), src.magic.size());

        if (locationNumber > magicNumber){
            if (locationNumber + (-1024) == magicNumber){
                auto&& data = new RealData(src.locationid, magicNumber);
                datas->push_back(data);
                //std::cout << src.locationid << ": " << locationNumber << " - " << magicNumber << std::endl;
                if (PostDig(src.locationid) == 0){
                    passCount++;
                }
                continue;
            }

            if ((locationNumber & 1023) == magicNumber){
                auto&& data = new RealData(src.locationid, magicNumber);
                datas->push_back(data);
                //std::cout << src.locationid << ": " << locationNumber << " % " << magicNumber << std::endl;
                if (PostDig(src.locationid) == 0){
                    passCount++;
                }
                continue;
            }
        }
        else{
            if (locationNumber + 1024 == magicNumber){
                auto&& data = new RealData(src.locationid, magicNumber);
                datas->push_back(data);
                //std::cout << src.locationid << ": " << locationNumber << " + " << magicNumber << std::endl;
                if (PostDig(src.locationid) == 0){
                    passCount++;
                }
                continue;
            }

            if ((locationNumber << 10) == magicNumber){
                auto&& data = new RealData(src.locationid, magicNumber);
                datas->push_back(data);
                //std::cout << src.locationid << ": " << locationNumber << " * " << magicNumber << std::endl;
                if (PostDig(src.locationid) == 0){
                    passCount++;
                }
                continue;
            }
        }
    }

    {
        std::scoped_lock<std::mutex> g(settleMutex);
        totalCount += passCount;
        if (datas->size() > 0){
            realDatas->insert(realDatas->end(), datas->begin(), datas->end());
        }
    }
    delete datas;
    memPool->Free(chunk);
}

int main(int argc, char const *argv[]){
    auto startMS = _567::NowMicroseconds();

    cpu_set_t mask;
    CPU_ZERO(&mask);                // 初始化set集，将set置为空
    CPU_SET(0, &mask);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1){
        std::cout << "Bind main thread to cpu 0 failed." << std::endl;
    }
    else{
        std::cout << "Bind main thread to cpu 0." << std::endl;
    }

    // 禁掉 SIGPIPE 信号避免因为连接关闭出错
    _567::IgnoreSignal();
    // 初始化线程池
    auto&& pool = std::make_shared<_567::ThreadPool<void>>(4);

    // 初始化内存池
    memPool = new MemPool();

    realDatas = new std::vector<RealData*>();

    // 从四个起点开始沿不同方向遍历所有文件
    int startPoint0 = 31, startPoint1 = 32, startPoint2 = 95, startPoint3 = 96;
    std::string file;
    for (int offset = 0; offset < 32; ++offset){
        int idx = startPoint0 - offset;
        file = "./Treasure_" + std::to_string(idx) + ".data";
        std::ifstream ifstream0(file);
        if (ifstream0.is_open()){
            std::string temp;
            auto&& chunk = memPool->Alloc();
            while (getline(ifstream0, temp)){
                auto&& data = chunk->datas[chunk->size];
                data.size = temp.size();
                memcpy(data.buffer, temp.c_str(), data.size);
                chunk->size++;
                ++totalLines;
                if (chunk->size >= MAXCHUNKSIZE){
                    pool->Add([chunk](){
                        HandleSourceData(chunk);
                    });
                    chunk = memPool->Alloc();
                }
            }
            if (chunk->size > 0){
                pool->Add([chunk](){
                    HandleSourceData(chunk);
                });
            }
            ifstream0.close();
            std::cout << file << " completed." << std::endl;
            usleep(500000);           // 休眠500000微秒(500毫秒)
        }
        else{
            std::cout << "File " << file << " open failed." << std::endl;
        }

        idx = startPoint1 + offset;
        file = "./Treasure_" + std::to_string(idx) + ".data";
        std::ifstream ifstream1(file);
        if (ifstream1.is_open()){
            std::string temp;
            auto&& chunk = memPool->Alloc();
            while (getline(ifstream1, temp)){
                auto&& data = chunk->datas[chunk->size];
                data.size = temp.size();
                memcpy(data.buffer, temp.c_str(), data.size);
                chunk->size++;
                ++totalLines;
                if (chunk->size >= MAXCHUNKSIZE){
                    pool->Add([chunk](){
                        HandleSourceData(chunk);
                    });
                    chunk = memPool->Alloc();
                }
            }
            if (chunk->size > 0){
                pool->Add([chunk](){
                    HandleSourceData(chunk);
                });
            }
            ifstream1.close();
            std::cout << file << " completed." << std::endl;
            usleep(500000);           // 休眠500000微秒(500毫秒)
        }
        else{
            std::cout << "File " << file << " open failed." << std::endl;
        }

        idx = startPoint2 - offset;
        file = "./Treasure_" + std::to_string(idx) + ".data";
        std::ifstream ifstream2(file);
        if (ifstream2.is_open()){
            std::string temp;
            auto&& chunk = memPool->Alloc();
            while (getline(ifstream2, temp)){
                auto&& data = chunk->datas[chunk->size];
                data.size = temp.size();
                memcpy(data.buffer, temp.c_str(), data.size);
                chunk->size++;
                ++totalLines;
                if (chunk->size >= MAXCHUNKSIZE){
                    pool->Add([chunk](){
                        HandleSourceData(chunk);
                    });
                    chunk = memPool->Alloc();
                }
            }
            if (chunk->size > 0){
                pool->Add([chunk](){
                    HandleSourceData(chunk);
                });
            }
            ifstream2.close();
            std::cout << file << " completed." << std::endl;
            usleep(500000);           // 休眠500000微秒(500毫秒)
        }
        else{
            std::cout << "File " << file << " open failed." << std::endl;
        }

        idx = startPoint3 + offset;
        file = "./Treasure_" + std::to_string(idx) + ".data";
        std::ifstream ifstream3(file);
        if (ifstream3.is_open()){
            std::string temp;
            auto&& chunk = memPool->Alloc();
            while (getline(ifstream3, temp)){
                auto&& data = chunk->datas[chunk->size];
                data.size = temp.size();
                memcpy(data.buffer, temp.c_str(), data.size);
                chunk->size++;
                ++totalLines;
                if (chunk->size >= MAXCHUNKSIZE){
                    pool->Add([chunk](){
                        HandleSourceData(chunk);
                    });
                    chunk = memPool->Alloc();
                }
            }
            if (chunk->size > 0){
                pool->Add([chunk](){
                    HandleSourceData(chunk);
                });
            }
            ifstream3.close();
            std::cout << file << " completed." << std::endl;
            usleep(500000);           // 休眠500000微秒(500毫秒)
        }
        else{
            std::cout << "File " << file << " open failed." << std::endl;
        }
    }

    // test 8 files
//    for (int i = 0; i < 8; ++i){
//        file = "./Treasure_" + std::to_string(i) + ".data";
//        std::ifstream ifstream(file);
//        if (!ifstream.is_open()){
//            std::cout << "file open failed." << std::endl;
//            return -1;
//        }
//        std::string temp;
//        auto&& chunk = memPool->Alloc();
//        while (getline(ifstream, temp)){
//            auto&& data = chunk->datas[chunk->size];
//            data.size = temp.size();
//            memcpy(data.buffer, temp.c_str(), data.size);
//            chunk->size++;
//            ++totalLines;
//            if (chunk->size >= MAXCHUNKSIZE){
//                //HandleSourceData(lines);
//                pool->Add([chunk](){
//                    HandleSourceData(chunk);
//                });
//                chunk = memPool->Alloc();
//            }
//        }
//        if (chunk->size > 0){
//            //HandleSourceData(lines);
//            pool->Add([chunk](){
//                HandleSourceData(chunk);
//            });
//        }
//        ifstream.close();
//        std::cout << file << " completed." << std::endl;
//        usleep(500000);           // 休眠500000微秒(500毫秒)
//    }

    while (pool->JobsCount() > 0){
        usleep(500000);           // 休眠500000微秒(500毫秒)
    }

    // 寻找公式


    std::cout << "find " << realDatas->size() << " datas in " << totalLines << " lines." << std::endl;
    std::cout << "final score: " << totalCount << std::endl;

    auto afterMS = _567::NowMicroseconds();
    auto diff = afterMS - startMS;
    std::cout << "total cost " << diff << "ms" << std::endl;
    std::cout << "total count in pool = " << memPool->TotalCount() << std::endl;

    delete memPool;
    for (int i = 0; i < realDatas->size(); ++i){
        delete (*realDatas)[i];
    }
    delete realDatas;
}