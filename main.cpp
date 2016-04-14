/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: d
 *
 * Created on April 10, 2016, 12:02 AM
 */

#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include "CompletionThreadPool.h"

using namespace std;

int f(int sleep_time) {
    sleep(sleep_time);
    return sleep_time;
}

int main(int argc, char** argv) {
    CompletionThreadPool<int> threadPool;
    threadPool.Submit(f, 4);
    threadPool.Submit(f, 1);

    std::future<int> first = threadPool.Take();
    std::future<int> second = threadPool.Take();
    
    std::cout << "first " << first.get() << std::endl;
    std::cout << "second " << second.get() << std::endl;
    
    return 0;
}

