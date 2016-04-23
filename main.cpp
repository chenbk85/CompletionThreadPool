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
    
    std::cout << "Test ThreadPool";
    ThreadPool threadPool;
    std::future<int> first = threadPool.Submit(f, 5);
    std::cout << "first " << first.get() << std::endl;
    
    std::future<int> second = threadPool.Submit(f, 1);
    std::cout << "second " << second.get() << std::endl;
   
    std::cout << "Test CompletionThreadPool";
    CompletionThreadPool<int> completionThreadPool;
    completionThreadPool.Submit(f, 5);
    completionThreadPool.Submit(f, 1);

    std::future<int> firstCompletion = completionThreadPool.Take();
    std::cout << "first " << firstCompletion.get() << std::endl;
    
    std::future<int> secondCompletion = completionThreadPool.Take();
    std::cout << "second " << secondCompletion.get() << std::endl;

    return 0;
}

