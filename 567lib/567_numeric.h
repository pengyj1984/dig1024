#pragma once

#include <bits/stdc++.h>
#include <cstdio>
#include <iostream>
#include <sstream>

// 重载 << 运算符支持 int128
std::ostream& operator << (std::ostream& dest, __int128_t value){
    std::ostream::sentry s(dest);
    if (s){
        __uint128_t tmp = value < 0 ? -value : value;
        char buffer[128];
        char *d = std::end(buffer);
        do{
            --d;
            *d = "0123456789"[tmp % 10];
            tmp /= 10;
        } while (tmp != 0);
        if (value < 0){
            --d;
            *d = '-';
        }
        int len = std::end(buffer) - d;
        if (dest.rdbuf()->sputn(d, len) != len){
            dest.setstate(std::ios_base::badbit);
        }
    }

  return dest;
}

// 重载 << 运算符支持 uint128
std::ostream& operator << (std::ostream& dest, __uint128_t value){
    std::ostream::sentry s(dest);
    if (s){
        __uint128_t tmp = value;
        char buffer[128];
        char *d = std::end(buffer);
        do{
            --d;
            *d = "0123456789"[tmp % 10];
            tmp /= 10;
        } while (tmp != 0);
        if (value < 0){
            --d;
            *d = '-';
        }
        int len = std::end(buffer) - d;
        if (dest.rdbuf()->sputn(d, len) != len){
            dest.setstate(std::ios_base::badbit);
        }
    }

    return dest;
}

namespace _567{

// 从标准Console窗口接收一个 int128
    inline __int128_t read(){
        __int128_t x = 0, f = 1;
        char ch = getchar();
        while (ch < '0' || ch > '9'){
            if (ch == '-') f = -1;
            ch = getchar();
        }
        while (ch >= '0' && ch <= '9'){
            x = x * 10 + (ch - '0');
            ch = getchar();
        }
        return x * f;
    }

// 解析字符串为 int128, 不考虑符号问题
    inline __int128_t parseFromCString(const char* str, int len){
        __int128_t x = 0;
        int i = 0;
        for (int i = 0; i < len; ++i){
            char ch = str[i];
            if (ch >= '0' && ch <= '9'){
                x = x * 10 + (ch - '0');
            }
        }
        return x;
    }

// 输出int128到Console
    inline void write(__int128_t x){
        if (x < 0){
            putchar('-');
            x = -x;
        }
        if (x > 9) write(x / 10);
        putchar(x % 10 + '0');
    }

    inline std::string writeToString(__int128_t x){
        std::stringstream ss;
        ss << x;
        return ss.str();
    }
}