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

int formulaScore = 0;
int totalCount = 0;
std::vector<RealData*> *realDatas;
std::unordered_map<std::string, int> usedDatas;
int usedCount = 0;

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

    struct sched_param s_parm;
    s_parm.sched_priority = sched_get_priority_max(SCHED_RR);
    sched_setscheduler(0, SCHED_RR, &s_parm);

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

// 2021.10.23
void MarkUsedDatas(){
    std::unordered_map<std::string, int>::iterator iter = usedDatas.begin();
    while (iter != usedDatas.end()){
        (*realDatas)[iter->second]->flag = 1;
        ++iter;
    }

    usedCount += usedDatas.size();
    usedDatas.clear();
}

// 2021.10.23
void MarkAbandonedDatas(std::vector<std::string> &vec){
    std::vector<std::string>::iterator iter = vec.begin();
    while (iter != vec.end()){
        auto it = usedDatas.find(*iter);
        if (it != usedDatas.end()){
            (*realDatas)[it->second]->flag = 1;
        }
        ++iter;
        ++usedCount;
    }
    usedDatas.clear();
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

    int index = 0;
    __int128_t currNumber = 0;
    int num = 0;
    std::string currFormula;
    while (true){
        while (realDatas->size() > 0 && index < realDatas->size()){
            auto data0 = (*realDatas)[index++];
            if (data0->flag != 0 || data0->magic == 0){
                continue;
            }

            if (data0->magic + currNumber <= 1024){
                ++num;
                currNumber += data0->magic;
                currFormula.append(data0->locationid);
                usedDatas[data0->locationid] = index - 1;
                if (currNumber == 1024 && num >= 4){
                    std::cout << "curr formula = " << currFormula << std::endl;
                    FormulaResult result;
                    int errorno = PostFormula(currFormula, result);
                    if (errorno == 0){
                        MarkUsedDatas();
                        formulaScore += (num * num);
                    }
                    else if (errorno == 3){
                        MarkAbandonedDatas(result.data);
                    }
                    num = 0;
                    currFormula.clear();
                    currNumber = 0;
                }
                else{
                    currFormula.append(" + ");
                }
            }
            else{
                if (index >= realDatas->size()) {
                    // 多线程程序, 这里continue不一定会退出上层while循环, 有可能此时正好其他线程的新数据来了;
                    // 此时之前的data0就被跳过了, 但是也无所谓, 以后还会循环过来的.
                    continue;
                }

                // 已经取了一个数出来, 这里不能再随意跳过了, 除非到最后, 否则必须找到一个来放公式中去
                auto data1 = (*realDatas)[index++];
                while (data1->flag != 0 || data1->magic == 0){
                    // 如果这里发现已经到队尾了, 直接退出while, 后面的 if 会 continue 上一层 while
                    if (index >= realDatas->size()) break;
                    data1 = (*realDatas)[index++];
                }
                if (data1->flag != 0 || data1->magic == 0){
                    continue;
                }

                if(data1->magic + currNumber <= 1024){
                    ++num;
                    currNumber += data1->magic;
                    currFormula.append(data1->locationid);
                    usedDatas[data1->locationid] = index - 1;
                    if (currNumber == 1024 && num >= 4){
                        std::cout << "curr formula = " << currFormula << std::endl;
                        FormulaResult result;
                        int errorno = PostFormula(currFormula, result);
                        if (errorno == 0){
                            MarkUsedDatas();
                            formulaScore += (num * num);
                        }
                        else if (errorno == 3){
                            MarkAbandonedDatas(result.data);
                        }
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
                num += 2;
                usedDatas[data0->locationid] = index - 2;
                usedDatas[data1->locationid] = index - 1;
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
                            FormulaResult result;
                            int errorno = PostFormula(currFormula, result);
                            if (errorno == 0){
                                MarkUsedDatas();
                                formulaScore += (num * num);
                            }
                            else if (errorno == 3){
                                MarkAbandonedDatas(result.data);
                            }
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
                            FormulaResult result;
                            int errorno = PostFormula(currFormula, result);
                            if (errorno == 0){
                                MarkUsedDatas();
                                formulaScore += (num * num);
                            }
                            else if (errorno == 3){
                                MarkAbandonedDatas(result.data);
                            }
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
                            FormulaResult result;
                            int errorno = PostFormula(currFormula, result);
                            if (errorno == 0){
                                MarkUsedDatas();
                                formulaScore += (num * num);
                            }
                            else if (errorno == 3){
                                MarkAbandonedDatas(result.data);
                            }
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
                            FormulaResult result;
                            int errorno = PostFormula(currFormula, result);
                            if (errorno == 0){
                                MarkUsedDatas();
                                formulaScore += (num * num);
                            }
                            else if (errorno == 3){
                                MarkAbandonedDatas(result.data);
                            }
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
        }

        // 清理掉当前数据, 从头再来
        index = 0;
        num = 0;
        currFormula.clear();
        currNumber = 0;
        usedDatas.clear();
        // 其他事情做完了, 剩余数字不足4个, 直接退出不管了
        if (pool->JobsCount() == 0 && ReadFileFinished() && usedCount >= realDatas->size() - 4) break;
        else usleep(1000000);                    // 睡１秒
    }

    std::cout << "Find " << realDatas->size() << " datas." << std::endl;
    std::cout << "Dig score: " << totalCount << std::endl;
    std::cout << "Formula score: " << formulaScore << std::endl;

    auto afterMS = _567::NowMicroseconds();
    auto diff = afterMS - startMS;
    std::cout << "Total cost " << diff << "ms" << std::endl;
    std::cout << "Max items num in pool = " << memPool->TotalCount() << std::endl;
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