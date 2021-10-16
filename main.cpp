#include <iostream>
#include <vector>
#include <unordered_map>
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
        ajson::load_from_buff(src, &temp.buffer[0], temp.size);
        __int128_t locationNumber = _567::parseFromCString(src.locationid.c_str(), src.locationid.size());
        __int128_t magicNumber = _567::parseFromCString(src.magic.c_str(), src.magic.size());

        if (locationNumber > magicNumber){
            if (locationNumber + (-1024) == magicNumber){
                auto&& data = new RealData(src.locationid, magicNumber);
                datas->push_back(data);
                if (PostDig(src.locationid) == 0){
                    passCount++;
                }
                continue;
            }

            if ((locationNumber & 1023) == magicNumber){
                auto&& data = new RealData(src.locationid, magicNumber);
                datas->push_back(data);
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
                if (PostDig(src.locationid) == 0){
                    passCount++;
                }
                continue;
            }

            if ((locationNumber << 10) == magicNumber){
                auto&& data = new RealData(src.locationid, magicNumber);
                datas->push_back(data);
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
        if (datas->size() > 0)
            realDatas->insert(realDatas->end(), datas->begin(), datas->end() - 1);
    }
    ///////
//    totalCount += datas->size();
//    if (datas->size() > 0){
//        std::cout << "find " << totalCount << " datas in " << totalLines << " lines."<< std::endl;
//    }
//    for (int i = 0; i < datas->size(); ++i){
//        delete (*datas)[i];
//    }
    delete datas;
    memPool->Free(chunk);
}

int main(int argc, char const *argv[]){
    auto startMS = _567::NowMicroseconds();

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
//    for (int offset = 0; offset < 32; ++offset){
//        int idx = startPoint0 - offset;
//        file = "./Treasure_" + std::to_string(idx) + ".data";
//        std::cout << file << std::endl;
//
//        idx = startPoint1 + offset;
//        file = "./Treasure_" + std::to_string(idx) + ".data";
//        std::cout << file << std::endl;
//
//        idx = startPoint2 - offset;
//        file = "./Treasure_" + std::to_string(idx) + ".data";
//        std::cout << file << std::endl;
//
//        idx = startPoint3 + offset;
//        file = "./Treasure_" + std::to_string(idx) + ".data";
//        std::cout << file << std::endl;
//    }

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
//        usleep(500);           // 休眠500微秒(0.5毫秒)
//    }

    while (pool->JobsCount() > 0){
        usleep(2000);           // 休眠2000微秒(2毫秒)
    }

////    ajson::save_to(ss, data);
////    std::cout << "json: " << ss.str() << std::endl;
    auto ret = PostDig("01mq1k08v4qxl8f8yy9677oup67klu3ydzhr7xixqdbmcaz3lfwdkk04dl58fk8d");

    std::cout << "find " << totalCount << " datas in " << totalLines << " lines." << std::endl;

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