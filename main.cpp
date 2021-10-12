#include <bits/stdc++.h>
#include <cstdio>
#include <iostream>

typedef long long ll;
#define INT __int128_t

std::ostream& operator << (std::ostream& dest, __int128_t value){
    std::ostream ::sentry s(dest);
    if (s){
        __uint128_t tmp = value < 0 ? -value : value;
        char buffer[128];
        char *d = std::end(buffer);
        do{
            --d;
            *d = "0123456789"[tmp % 10];
            tmp / 10;
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

inline void write(INT x){
    if (x < 0){
        putchar('-');
        x = -x;
    }
    if (x > 9) write(x / 10);
    putchar(x % 10 + '0');
}

int main(int argc, char const *argv[]){
    INT number = 9 * 1e20;//99999999999999999999999999999999L;
    number *= 1e12;
    std::cout << sizeof(number) << std::endl;
    write(number);
    //std::cout << number << std::endl;
}