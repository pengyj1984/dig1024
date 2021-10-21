#pragma once

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sys/mman.h>
#include "mempool.h"

void ReadFileUseStream(std::string fileName, MemPool *pool){
    std::ifstream ifstream(fileName);
    int lines = 0;
    if (ifstream.is_open()){
        std::string temp;
        auto&& chunk = pool->Alloc();
        while (getline(ifstream, temp)){
            auto&& data = chunk->datas[chunk->size];
            data.size = temp.size();
            memcpy(data.buffer, temp.c_str(), data.size);
            ++(chunk->size);
            ++lines;
            if (chunk->size >= MAXCHUNKSIZE){
                chunk = pool->Alloc();
            }
        }

        if (chunk->size > 0){
            /////
        }

        ifstream.close();
    }
    std::cout << "total lines = " << lines << std::endl;
}

void ReadFileUseFile(const char *fileName, MemPool *pool){
    FILE *file = fopen(fileName, "rb");
    int lines = 0;

    fseek(file, 0, 2);
    int filelen = ftell(file);
    fseek(file, 0, 0);

    char *content = (char*)malloc(filelen * sizeof (char));
    fread(content, filelen, 1, file);
    unsigned long prevIdx = 0;
    auto&& chunk = pool->Alloc();
    for (unsigned long i = 0; i < filelen; ++i){
        if (content[i] == '\n'){
            auto&& data = chunk->datas[chunk->size];
            int size = i - prevIdx + 1;
            data.size = size;
            memcpy(data.buffer, &content[prevIdx], data.size);
            ++(chunk->size);
            ++lines;
            if (chunk->size >= MAXCHUNKSIZE){
                chunk = pool->Alloc();
            }
            prevIdx = i;
        }
    }

    if (chunk->size > 0){
        /////
    }

    free(content);
    fclose(file);
    std::cout << "total lines = " << lines << std::endl;
}

void ReadFileUsemmap(const char *fileName, MemPool *pool){
    char *data = nullptr;
    //int fd = open();
}