#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "567_chrono.h"
#include "567_numeric.h"
#include "567_signal.h"
#include "567_threadpool.h"
#include "curl_helper.h"
#include "mempool.h"
#include "read_file_test.h"

int main(int argc, char const *argv[]){
    auto startMS = _567::NowMicroseconds();
    int cpus = sysconf(_SC_NPROCESSORS_ONLN);       // 获取当前设备cpu数量
    // 禁掉 SIGPIPE 信号避免因为连接关闭出错
    _567::IgnoreSignal();

    // 初始化内存池
    auto&& memPool = new MemPool();

    //std::ifstream if0("./Treasure_0.data");
    //std::cout << "file0 size = " << if0.width() << std::endl;
    //std::ifstream if1("./Treasure_1.data");
    //std::cout << "file1 size = " << if1.width() << std::endl;
    //std::ifstream if2("./Treasure_2.data");
    //std::cout << "file2 size = " << if2.width() << std::endl;
    //std::ifstream if3("./Treasure_3.data");
    //std::cout << "file3 size = " << if3.width() << std::endl;

    for (int i = 0; i < 128; ++i){
        std::string file = "./Treasure_";
        file.append(std::to_string(i));
        file.append(".data");
        ReadFileUseStream(file.c_str(), memPool);
    }
//    ReadFileUseStream("./Treasure_0.data", memPool);
//    ReadFileUseStream("./Treasure_1.data", memPool);
//    ReadFileUseStream("./Treasure_2.data", memPool);
//    ReadFileUseStream("./Treasure_3.data", memPool);
//    ReadFileUseFile("./Treasure_0.data", memPool);
//    ReadFileUseFile("./Treasure_1.data", memPool);
//    ReadFileUseFile("./Treasure_2.data", memPool);
//    ReadFileUseFile("./Treasure_3.data", memPool);
//    ReadFileUsemmap("./Treasure_0.data", memPool);
//    ReadFileUsemmap("./Treasure_1.data", memPool);
//    ReadFileUsemmap("./Treasure_2.data", memPool);
//    ReadFileUsemmap("./Treasure_3.data", memPool);

//    FILE *file0 = fopen("./Treasure_0.data", "r");
//    fseek(file0, 0, 2);
//    fseek(file0, 0, 0);
//    int file0len = ftell(file0);
//    char *content0 = (char*)malloc(file0len * sizeof (char));
//    fread(content0, file0len, 1, file0);

    auto endMS = _567::NowMicroseconds();
    std::cout << "time = " << (endMS - startMS) << "ms." << std::endl;
    //std::cout << "pool size = " << memPool->TotalCount() << std::endl;
}