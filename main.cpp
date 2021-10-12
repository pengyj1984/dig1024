#include <iostream>
#include "ajson.hpp"
#include "567_chrono.h"
#include "567_numeric.h"
#include "567_signal.h"
#include "567_threadpool.h"


int main(int argc, char const *argv[]){
    // 禁掉 SIGPIPE 信号避免因为连接关闭出错
    _567::IgnoreSignal();

    auto&& pool = std::make_shared<_567::ThreadPool<void>>(4);

    __int128_t number = 9 * 1e16;//99999999999999999999999999999999L;

    __int128_t number2 = 1e16;
    number *= number2;
    write(number);
    std::cout << std::endl;
    std::cout << number << std::endl;
}