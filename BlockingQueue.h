#pragma once

#include <future>

template <typename T>
class BlockingQueue {
public:

    ~BlockingQueue() {

    }

    void put(std::future<T>&& future) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(std::move(future));
        lock.unlock();
        notify_.notify_one();
    }

    std::future<T> take() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.empty()) {
            notify_.wait(lock);
        }
        std::future<T> future = std::move(queue_.front());
        queue_.pop();
        return future;
    }

private:
    std::condition_variable notify_;
    std::queue<std::future<T>> queue_;
    std::mutex mutex_;
};
