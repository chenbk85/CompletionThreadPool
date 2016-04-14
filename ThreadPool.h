#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <tuple>
#include <future>
#include <atomic>

#include "BlockingQueue.h"

class ThreadPool {
public:
    ThreadPool(int maxthreads);

    virtual ~ThreadPool();

    template<typename Fn, typename... Args, typename T = typename std::result_of<Fn(Args...)>::type>
    std::future<T> Submit(Fn &&fn, Args &&... args);

    void Shutdown();


protected:

    template<typename Fn, typename... Args, typename T = typename std::result_of<Fn(Args...)>::type>
    void SubmitToListener(BlockingQueue<T>* b, Fn&& fn, Args&&... args);


private:
    void SpawnThread();
    std::function<void() > GetNextTask();
    void Main();

    template<typename F, typename R>
    void SetPromiseValue(std::promise<R> & p, F && f);

    template<typename F>
    void SetPromiseValue(std::promise<void> & p, F && f);

private:
    int maxthreads_;
    std::vector<std::thread> threads_;
    std::queue<std::function<void() >> tasks_;
    std::mutex mutex_;
    std::condition_variable notify_;
    std::atomic<bool> flag_terminate_threads_;

};

ThreadPool::ThreadPool(int maxthreads = std::thread::hardware_concurrency())
: flag_terminate_threads_(false), maxthreads_(maxthreads) {
    std::cout << "ThreadPool with " << maxthreads << " maximum threads" << std::endl;
}

ThreadPool::~ThreadPool() {
    Shutdown();
}

void ThreadPool::Shutdown() {
    {
        std::unique_lock<std::mutex> lock(mutex_);

        flag_terminate_threads_ = true;
        notify_.notify_all();
    }

    for (std::thread & thread : threads_) {
        thread.join();
    }

    threads_.clear();
}

template<typename Fn, typename... Args, typename T>
std::future<T> ThreadPool::Submit(Fn&& fn, Args&&... args) {
    std::unique_lock<std::mutex> lock(mutex_);

    // If there are already waiting tasks try to spawn one more worker thread
    if (!tasks_.empty() || threads_.empty())
        SpawnThread();

    auto call = std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...);
    std::shared_ptr<std::promise < T>> promise = std::make_shared<std::promise < T >> ();

    std::future<T> future = promise->get_future();
    std::function<void() > wrapper = [this, call, promise]() {
        try {
            SetPromiseValue(*promise, call);
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    };

    tasks_.emplace(std::move(wrapper));
    notify_.notify_one();
    return future;
}

template<typename Fn, typename... Args, typename T>
void ThreadPool::SubmitToListener(BlockingQueue<T>* b, Fn&& fn, Args&&... args) {
    std::unique_lock<std::mutex> lock(mutex_);

    // If there are already waiting tasks try to spawn one more worker thread
    if (!tasks_.empty() || threads_.empty())
        SpawnThread();

    auto call = std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...);
    std::shared_ptr<std::promise < T>> promise = std::make_shared<std::promise < T >> ();


    std::function<void() > wrapper = [this, call, promise, b]() {
        try {
            SetPromiseValue(*promise, call);
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
        b->put(promise->get_future());
    };

    tasks_.emplace(std::move(wrapper));
    notify_.notify_one();
}

template<typename F, typename R>
void ThreadPool::SetPromiseValue(std::promise<R> & p, F && f) {
    p.set_value(f());
}

template<typename F>
void ThreadPool::SetPromiseValue(std::promise<void> & p, F && f) {
    f();
    p.set_value();
}

void ThreadPool::SpawnThread() {
    if (maxthreads_ > threads_.size()) {
        threads_.emplace_back(&ThreadPool::Main, this);
    }
}

std::function<void() > ThreadPool::GetNextTask() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!flag_terminate_threads_) {
        if (!tasks_.empty()) {
            std::function<void() > task = tasks_.front();
            tasks_.pop();
            return task;
        }
        notify_.wait(lock);
    }
    return []() {
    };
}

void ThreadPool::Main() {
    while (!flag_terminate_threads_) {
        std::function<void() > task = GetNextTask();
        task();
    }
}


