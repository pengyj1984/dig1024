#pragma once

#include "ajson.hpp"
#include <iostream>

// 原始数据, 待解析
struct SourceData{
    std::string locationid;
    std::string magic;
};

AJSON(SourceData, locationid, magic);

// 解析后的数据
struct RealData{
    RealData(const std::string &_locationid, const __uint128_t &_magic);
    std::string locationid;
    __uint128_t magic;
};

RealData::RealData(const std::string &_locationid, const __uint128_t &_magic):locationid(_locationid), magic(_magic){}