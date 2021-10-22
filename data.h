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
    RealData(const std::string &_locationid, const __int128_t &_magic);
    std::string locationid;
    __int128_t magic;
    int flag;
};

RealData::RealData(const std::string &_locationid, const __int128_t &_magic):locationid(_locationid), magic(_magic), flag(0){
}

// 挖宝验证数据结构
struct DigData{
    DigData(const std::string &_locationid, const std::string &_token);
    std::string locationid;
    std::string token;
};

DigData::DigData(const std::string &_locationid, const std::string &_token):locationid(_locationid), token(_token){}

AJSON(DigData, locationid, token);

// 挖宝返回数据结构
struct DigResult{
    int errorno;
    std::string msg;
    std::vector<std::string> data;
};

AJSON(DigResult, errorno, msg, data);

// 公式验证数据结构
struct FormulaData{
    FormulaData(const std::string &_formula, const std::string &_token);
    std::string formula;
    std::string token;
};

FormulaData::FormulaData(const std::string &_formula, const std::string &_token):formula(_formula), token(_token){}

AJSON(FormulaData, formula, token);

// 公式返回数据结构
struct FormulaResult{
    int errorno;
    std::vector<std::string> data;
};

AJSON(FormulaResult, errorno, data);