#pragma once
#include "ThreadPool.h"
#include "BlockingQueue.h"

/**
 * This thread pool returns results of submitted tasks in order of their 
 * completion.
 */
template <typename T>
class CompletionThreadPool : private ThreadPool {
public:

    CompletionThreadPool(int maxthreads = std::thread::hardware_concurrency()) 
            : ThreadPool(maxthreads), blocking_queue_(new BlockingQueue<T>()) {
    }

    ~CompletionThreadPool() { 
    }

    /**
     * Submit new task to the pool
     * @param fn function
     * @param args function arguments
     */
    template<typename Fn, typename... Args>
    bool Submit(Fn &&fn, Args &&... args);

    /**
     * @return future for the first completed task
     */
    std::future<T> Take();

private:
    const std::shared_ptr<BlockingQueue<T>> blocking_queue_;
};

template<typename T>
template<typename Fn, typename... Args>
bool CompletionThreadPool<T>::Submit(Fn &&fn, Args &&... args) {
    return ThreadPool::Submit(blocking_queue_, fn, args...);
}

template<typename T>
std::future<T> CompletionThreadPool<T>::Take() {
    return blocking_queue_->take();
}
