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

#define MAXFILENUMPERTHREAD 32

std::mutex settleMutex;
MemPool *memPool;
std::shared_ptr<_567::ThreadPool<void>> pool;

int filePipes[4] = {0, 0, 0, 0};

bool stopFormula = false;
bool stopped = false;

int formulaScore = 0;
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

void ReadFiles(int start, int step, int index, int threadIdx){
    cpu_set_t mask;
    CPU_ZERO(&mask);                // 初始化set集，将set置为空
    CPU_SET(threadIdx, &mask);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1){
        //std::cout << "Bind thread to cpu " << cpu << " failed." << std::endl;
    }
    else{
        //std::cout << "Bind thread to cpu " << cpu << std::endl;
    }

    std::string file;
    while(filePipes[index] < MAXFILENUMPERTHREAD){
        file = "./Treasure_" + std::to_string(start) + ".data";
        std::ifstream ifstream(file);
        if (ifstream.is_open()){
            std::string temp;
            auto&& chunk = memPool->Alloc();
            while (getline(ifstream, temp)){
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
            ifstream.close();
            std::cout << file << " completed." << std::endl;
        }
        ++filePipes[index];
        start += step;
    }
}

bool ReadFileFinished(){
    return filePipes[0] >= MAXFILENUMPERTHREAD && filePipes[1] >= MAXFILENUMPERTHREAD
    && filePipes[2] >= MAXFILENUMPERTHREAD && filePipes[3] >= MAXFILENUMPERTHREAD;
}

void FindFormula(){
    auto index = 0;
    __int128_t currNumber = 0;
    int num = 0;
    std::string currFormula;
    STARTSEARCH:
    // 因为后面计算的时候是两个两个取出来算的, 这里如果正常判断最后可能会死循环
    if (index >= realDatas->size())
    {
        if (stopFormula) {
            return;
        }
        usleep(1000000);                // 睡１秒
    }

    while (index < realDatas->size() - 1){
        std::cout << "currNumber = " << currNumber << std::endl;
        auto data0 = (*realDatas)[index];
        if (data0->magic == 0){
            // 以防万一
            ++index;
            continue;
        }

        if (data0->magic + currNumber <= 1024){
            ++num;
            ++index;
            currNumber += data0->magic;
            currFormula.append(data0->locationid);
            if (currNumber == 1024 && num >= 4){
                std::cout << "curr formula = " << currFormula << std::endl;
                if (PostFormula(currFormula) == 0){
                    formulaScore += (num * num);
                }
                // 不管什么结果, 之前的全部不用了
                num = 0;
                currFormula.clear();
                currNumber = 0;
            }
            else{
                currFormula.append(" + ");
            }
            continue;
        }

        auto data1 = (*realDatas)[index + 1];
        if (data1->magic == 0){
            // 以防万一
            index += 2;
            continue;
        }

        if(data1->magic + currNumber <= 1024){
            ++num;
            index += 2;             // 直接跳到后面一个去
            currNumber += data1->magic;
            currFormula.append(data1->locationid);
            if (currNumber == 1024 && num >= 4){
                std::cout << "curr formula = " << currFormula << std::endl;
                if (PostFormula(currFormula) == 0){
                    formulaScore += (num * num);
                }
                // 不管什么结果, 之前的全部不用了
                num = 0;
                currFormula.clear();
                currNumber = 0;
            }
            else{
                currFormula.append(" + ");
            }
            continue;
        }

        // 到这里就必须把两个数一起处理了
        index += 2;             // 直接跳到后面一个去
        num += 2;
        if (data0->magic >= data1->magic){
            // 先尝试减法
            __int128_t diff = data0->magic - data1->magic;
            if(diff + currNumber <= 1024){
                currNumber += diff;
                currFormula.append(data0->locationid);
                currFormula.append(" - ");
                currFormula.append(data1->locationid);
                if (currNumber == 1024 && num >= 4){
                    std::cout << "curr formula = " << currFormula << std::endl;
                    if (PostFormula(currFormula) == 0){
                        formulaScore += (num * num);
                    }
                    // 不管什么结果, 之前的全部不用了
                    num = 0;
                    currFormula.clear();
                    currNumber = 0;
                }
                else{
                    currFormula.append(" + ");
                }

                continue;
            }

            __int128_t division = data0->magic / data1->magic;
            if (division + currNumber <= 1024){
                currNumber += division;
                currFormula.append(data0->locationid);
                currFormula.append(" / ");
                currFormula.append(data1->locationid);
                if (currNumber == 1024 && num >= 4){
                    std::cout << "curr formula = " << currFormula << std::endl;
                    if (PostFormula(currFormula) == 0){
                        formulaScore += (num * num);
                    }
                    // 不管什么结果, 之前的全部不用了
                    num = 0;
                    currFormula.clear();
                    currNumber = 0;
                }
                else{
                    currFormula.append(" + ");
                }
            }
            else{
                currFormula.append(data1->locationid);
                currFormula.append(" / ");
                currFormula.append(data0->locationid);
                currFormula.append(" + ");
            }
        }
        else{
            // 先尝试减法
            __int128_t diff = data1->magic - data0->magic;
            if(diff + currNumber <= 1024){
                currNumber += diff;
                currFormula.append(data1->locationid);
                currFormula.append(" - ");
                currFormula.append(data0->locationid);
                if (currNumber == 1024 && num >= 4){
                    std::cout << "curr formula = " << currFormula << std::endl;
                    if (PostFormula(currFormula) == 0){
                        formulaScore += (num * num);
                    }
                    // 不管什么结果, 之前的全部不用了
                    num = 0;
                    currFormula.clear();
                    currNumber = 0;
                }
                else{
                    currFormula.append(" + ");
                }

                continue;
            }

            __int128_t division = data1->magic / data0->magic;
            if (division + currNumber <= 1024){
                currNumber += division;
                currFormula.append(data1->locationid);
                currFormula.append(" / ");
                currFormula.append(data0->locationid);
                if (currNumber == 1024 && num >= 4){
                    std::cout << "curr formula = " << currFormula << std::endl;
                    if (PostFormula(currFormula) == 0){
                        formulaScore += (num * num);
                    }
                    // 不管什么结果, 之前的全部不用了
                    num = 0;
                    currFormula.clear();
                    currNumber = 0;
                }
                else{
                    currFormula.append(" + ");
                }
            }
            else{
                currFormula.append(data0->locationid);
                currFormula.append(" / ");
                currFormula.append(data1->locationid);
                currFormula.append(" + ");
            }
        }
    }

    if (!stopFormula)
        usleep(1000000);                    // 睡１秒
    else
        return;
    goto STARTSEARCH;
}

int main(int argc, char const *argv[]){
    auto startMS = _567::NowMicroseconds();
    int cpus = sysconf(_SC_NPROCESSORS_ONLN);       // 获取当前设备cpu数量
    // 禁掉 SIGPIPE 信号避免因为连接关闭出错
    _567::IgnoreSignal();
    // 初始化线程池．
    // 考虑发起curl请求的时候会等待, 线程池应该比实际cpu线程数大．
    pool = std::make_shared<_567::ThreadPool<void>>(12);

    // 初始化内存池
    memPool = new MemPool();

    realDatas = new std::vector<RealData*>();

    // 单独起个线程跑公式
//    std::thread thread([](){
//        int cpus = sysconf(_SC_NPROCESSORS_ONLN);       // 获取当前设备cpu数量
//        cpu_set_t mask;
//        CPU_ZERO(&mask);                // 初始化set集，将set置为空
//        if (cpus > 0)
//            CPU_SET(1, &mask);
//        else
//            CPU_SET(0, &mask);
//        if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1){
//            //std::cout << "Bind thread to cpu " << cpu << " failed." << std::endl;
//        }
//        else{
//            //std::cout << "Bind thread to cpu " << cpu << std::endl;
//        }
//        FindFormula();
//        stopped = true;
//    });

    // 从四个起点开始沿不同方向遍历所有文件
    std::thread readThread0([](){
        ReadFiles(31, -1, 0, 0);
    });
    std::thread readThread1([cpus](){
        ReadFiles(32, 1, 1, 1 % cpus);
    });
    std::thread readThread2([cpus](){
        ReadFiles(95, -1, 2, 2 % cpus);
    });
    std::thread readThread3([cpus](){
        ReadFiles(96, 1, 3, 3 % cpus);
    });
//    int startPoint0 = 31, startPoint1 = 32, startPoint2 = 95, startPoint3 = 96;
//    std::string file;
//    for (int offset = 0; offset < 32; ++offset){
//        int idx = startPoint0 - offset;
//        file = "./Treasure_" + std::to_string(idx) + ".data";
//        std::ifstream ifstream0(file);
//        if (ifstream0.is_open()){
//            std::string temp;
//            auto&& chunk = memPool->Alloc();
//            while (getline(ifstream0, temp)){
//                auto&& data = chunk->datas[chunk->size];
//                data.size = temp.size();
//                memcpy(data.buffer, temp.c_str(), data.size);
//                chunk->size++;
//                ++totalLines;
//                if (chunk->size >= MAXCHUNKSIZE){
//                    pool->Add([chunk](){
//                        HandleSourceData(chunk);
//                    });
//                    chunk = memPool->Alloc();
//                }
//            }
//            if (chunk->size > 0){
//                pool->Add([chunk](){
//                    HandleSourceData(chunk);
//                });
//            }
//            ifstream0.close();
//            std::cout << file << " completed." << std::endl;
//            //usleep(1000);           // 休眠1000微秒(1毫秒)
//        }
//
//        idx = startPoint1 + offset;
//        file = "./Treasure_" + std::to_string(idx) + ".data";
//        std::ifstream ifstream1(file);
//        if (ifstream1.is_open()){
//            std::string temp;
//            auto&& chunk = memPool->Alloc();
//            while (getline(ifstream1, temp)){
//                auto&& data = chunk->datas[chunk->size];
//                data.size = temp.size();
//                memcpy(data.buffer, temp.c_str(), data.size);
//                chunk->size++;
//                ++totalLines;
//                if (chunk->size >= MAXCHUNKSIZE){
//                    pool->Add([chunk](){
//                        HandleSourceData(chunk);
//                    });
//                    chunk = memPool->Alloc();
//                }
//            }
//            if (chunk->size > 0){
//                pool->Add([chunk](){
//                    HandleSourceData(chunk);
//                });
//            }
//            ifstream1.close();
//            std::cout << file << " completed." << std::endl;
//            //usleep(1000);           // 休眠1000微秒(1毫秒)
//        }
//
//        idx = startPoint2 - offset;
//        file = "./Treasure_" + std::to_string(idx) + ".data";
//        std::ifstream ifstream2(file);
//        if (ifstream2.is_open()){
//            std::string temp;
//            auto&& chunk = memPool->Alloc();
//            while (getline(ifstream2, temp)){
//                auto&& data = chunk->datas[chunk->size];
//                data.size = temp.size();
//                memcpy(data.buffer, temp.c_str(), data.size);
//                chunk->size++;
//                ++totalLines;
//                if (chunk->size >= MAXCHUNKSIZE){
//                    pool->Add([chunk](){
//                        HandleSourceData(chunk);
//                    });
//                    chunk = memPool->Alloc();
//                }
//            }
//            if (chunk->size > 0){
//                pool->Add([chunk](){
//                    HandleSourceData(chunk);
//                });
//            }
//            ifstream2.close();
//            std::cout << file << " completed." << std::endl;
//            //usleep(1000);           // 休眠1000微秒(1毫秒)
//        }
//
//        idx = startPoint3 + offset;
//        file = "./Treasure_" + std::to_string(idx) + ".data";
//        std::ifstream ifstream3(file);
//        if (ifstream3.is_open()){
//            std::string temp;
//            auto&& chunk = memPool->Alloc();
//            while (getline(ifstream3, temp)){
//                auto&& data = chunk->datas[chunk->size];
//                data.size = temp.size();
//                memcpy(data.buffer, temp.c_str(), data.size);
//                chunk->size++;
//                ++totalLines;
//                if (chunk->size >= MAXCHUNKSIZE){
//                    pool->Add([chunk](){
//                        HandleSourceData(chunk);
//                    });
//                    chunk = memPool->Alloc();
//                }
//            }
//            if (chunk->size > 0){
//                pool->Add([chunk](){
//                    HandleSourceData(chunk);
//                });
//            }
//            ifstream3.close();
//            std::cout << file << " completed." << std::endl;
//            //usleep(1000);           // 休眠1000微秒(1毫秒)
//        }
//    }

    int index = 0;
    __int128_t currNumber = 0;
    int num = 0;
    std::string currFormula;
    while (true){
//        if(index >= realDatas->size()){
//            if (pool->JobsCount() == 0 && ReadFileFinished()) break;
//            else usleep(1000000);                    // 睡１秒
//        }

        while (realDatas->size() > 0 && index < realDatas->size() - 1){
            std::cout << "currNumber = " << currNumber << std::endl;
            auto data0 = (*realDatas)[index];
            if (data0->magic == 0){
                // 以防万一
                ++index;
                continue;
            }

            if (data0->magic + currNumber <= 1024){
                ++num;
                ++index;
                currNumber += data0->magic;
                currFormula.append(data0->locationid);
                if (currNumber == 1024 && num >= 4){
                    std::cout << "curr formula = " << currFormula << std::endl;
                    if (PostFormula(currFormula) == 0){
                        formulaScore += (num * num);
                    }
                    // 不管什么结果, 之前的全部不用了
                    num = 0;
                    currFormula.clear();
                    currNumber = 0;
                }
                else{
                    currFormula.append(" + ");
                }
                continue;
            }

            auto data1 = (*realDatas)[index + 1];
            if (data1->magic == 0){
                // 以防万一
                index += 2;
                continue;
            }

            if(data1->magic + currNumber <= 1024){
                ++num;
                index += 2;             // 直接跳到后面一个去
                currNumber += data1->magic;
                currFormula.append(data1->locationid);
                if (currNumber == 1024 && num >= 4){
                    std::cout << "curr formula = " << currFormula << std::endl;
                    if (PostFormula(currFormula) == 0){
                        formulaScore += (num * num);
                    }
                    // 不管什么结果, 之前的全部不用了
                    num = 0;
                    currFormula.clear();
                    currNumber = 0;
                }
                else{
                    currFormula.append(" + ");
                }
                continue;
            }

            // 到这里就必须把两个数一起处理了
            index += 2;             // 直接跳到后面一个去
            num += 2;
            if (data0->magic >= data1->magic){
                // 先尝试减法
                __int128_t diff = data0->magic - data1->magic;
                if(diff + currNumber <= 1024){
                    currNumber += diff;
                    currFormula.append(data0->locationid);
                    currFormula.append(" - ");
                    currFormula.append(data1->locationid);
                    if (currNumber == 1024 && num >= 4){
                        std::cout << "curr formula = " << currFormula << std::endl;
                        if (PostFormula(currFormula) == 0){
                            formulaScore += (num * num);
                        }
                        // 不管什么结果, 之前的全部不用了
                        num = 0;
                        currFormula.clear();
                        currNumber = 0;
                    }
                    else{
                        currFormula.append(" + ");
                    }

                    continue;
                }

                __int128_t division = data0->magic / data1->magic;
                if (division + currNumber <= 1024){
                    currNumber += division;
                    currFormula.append(data0->locationid);
                    currFormula.append(" / ");
                    currFormula.append(data1->locationid);
                    if (currNumber == 1024 && num >= 4){
                        std::cout << "curr formula = " << currFormula << std::endl;
                        if (PostFormula(currFormula) == 0){
                            formulaScore += (num * num);
                        }
                        // 不管什么结果, 之前的全部不用了
                        num = 0;
                        currFormula.clear();
                        currNumber = 0;
                    }
                    else{
                        currFormula.append(" + ");
                    }
                }
                else{
                    currFormula.append(data1->locationid);
                    currFormula.append(" / ");
                    currFormula.append(data0->locationid);
                    currFormula.append(" + ");
                }
            }
            else{
                // 先尝试减法
                __int128_t diff = data1->magic - data0->magic;
                if(diff + currNumber <= 1024){
                    currNumber += diff;
                    currFormula.append(data1->locationid);
                    currFormula.append(" - ");
                    currFormula.append(data0->locationid);
                    if (currNumber == 1024 && num >= 4){
                        std::cout << "curr formula = " << currFormula << std::endl;
                        if (PostFormula(currFormula) == 0){
                            formulaScore += (num * num);
                        }
                        // 不管什么结果, 之前的全部不用了
                        num = 0;
                        currFormula.clear();
                        currNumber = 0;
                    }
                    else{
                        currFormula.append(" + ");
                    }

                    continue;
                }

                __int128_t division = data1->magic / data0->magic;
                if (division + currNumber <= 1024){
                    currNumber += division;
                    currFormula.append(data1->locationid);
                    currFormula.append(" / ");
                    currFormula.append(data0->locationid);
                    if (currNumber == 1024 && num >= 4){
                        std::cout << "curr formula = " << currFormula << std::endl;
                        if (PostFormula(currFormula) == 0){
                            formulaScore += (num * num);
                        }
                        // 不管什么结果, 之前的全部不用了
                        num = 0;
                        currFormula.clear();
                        currNumber = 0;
                    }
                    else{
                        currFormula.append(" + ");
                    }
                }
                else{
                    currFormula.append(data0->locationid);
                    currFormula.append(" / ");
                    currFormula.append(data1->locationid);
                    currFormula.append(" + ");
                }
            }
        }

        // 其他事情做完了, 就差最后一位数字, 直接退出不管了
        if (pool->JobsCount() == 0 && ReadFileFinished()) break;
        else usleep(1000000);                    // 睡１秒
    }

//    while (pool->JobsCount() > 0 || !ReadFileFinished()){
//        usleep(500000);           // 休眠500000微秒(500毫秒)
//    }

//    std::cout << "jobs done...wait formula" << std::endl;
//    stopFormula = true;
//    while (!stopped){
//        usleep(500000);           // 休眠500000微秒(500毫秒)
//    }
    //thread.join();

    std::cout << "find " << realDatas->size() << " datas in " << totalLines << " lines." << std::endl;
    std::cout << "dig score: " << totalCount << std::endl;
    std::cout << "formula score: " << formulaScore << std::endl;

    auto afterMS = _567::NowMicroseconds();
    auto diff = afterMS - startMS;
    std::cout << "total cost " << diff << "ms" << std::endl;
    std::cout << "total count in pool = " << memPool->TotalCount() << std::endl;
    std::cout << "I am the Riftbreaker." << std::endl;

    readThread0.join();
    readThread1.join();
    readThread2.join();
    readThread3.join();
    delete memPool;
    for (int i = 0; i < realDatas->size(); ++i){
        delete (*realDatas)[i];
    }
    delete realDatas;
}