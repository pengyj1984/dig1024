#pragma once

#include <mutex>
#include <vector>

#define MAXDATASIZE 128
#define MAXCHUNKSIZE 1024
#define INITPOOLSIZE 2048

struct MemData{
    char buffer[MAXDATASIZE];
    size_t size;
};

struct MemChunk{
    MemData datas[MAXCHUNKSIZE];
    size_t size;
    int index;
    int next;
};

class MemPool{
    std::vector<MemChunk*> *items;

    std::mutex mtx;
    std::condition_variable cond;

    int freeList;           // 空闲列表头
    int count;              // 当前以分配数量

public:
    MemPool(){
        items = new std::vector<MemChunk*>(INITPOOLSIZE);
        for (int i = 0; i < INITPOOLSIZE; ++i){
            (*items)[i] = new MemChunk();
            (*items)[i]->index = i;
            (*items)[i]->next = i + 1;
        }
        (*items)[INITPOOLSIZE - 1]->next = -1;
        freeList = 0;
        count = INITPOOLSIZE;
    }

    ~MemPool(){
        size_t size = items->size();
        for (int i = 0; i < size; ++i){
            delete (*items)[i];
        }
        delete items;
    }

    MemChunk* Alloc(){
        {
            std::unique_lock<std::mutex> lock(this->mtx);
            //this->cond.wait_for(lock, std::chrono::microseconds(10));
            if (freeList == -1){
                auto&& item = new MemChunk();
                item->index = items->size();
                item->next = -1;
                item->size = 0;
                items->push_back(item);
                count++;
                return item;
            }
            else{
                auto&& item = (*items)[freeList];
                freeList = item->next;
                item->size = 0;
                return item;
            }
        }
    }

    void Free(MemChunk *item){
        {
            std::unique_lock<std::mutex> lock(this->mtx);
            //this->cond.wait_for(lock, std::chrono::microseconds(10));
            item->next = freeList;
            freeList = item->index;
        }
    }

    int TotalCount(){
        return count;
    }
};