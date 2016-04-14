#pragma once
#include "ThreadPool.h"
#include "BlockingQueue.h"

template <typename T>
class CompletionThreadPool : private ThreadPool {
public:

    CompletionThreadPool(int maxthreads = std::thread::hardware_concurrency()) : ThreadPool(maxthreads) {
    }

    ~CompletionThreadPool() {
        Shutdown();
    }

    template<typename Fn, typename... Args>
    void Submit(Fn &&fn, Args &&... args);

    std::future<T> Take();

private:
    BlockingQueue<T> b_;
};

template<typename T>
template<typename Fn, typename... Args>
void CompletionThreadPool<T>::Submit(Fn &&fn, Args &&... args) {
    SubmitToListener(&b_, fn, args...);
}

template<typename T>
std::future<T> CompletionThreadPool<T>::Take() {
    return b_.take();
}
