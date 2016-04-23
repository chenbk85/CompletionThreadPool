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

/**
 * General purpose thread pool with bounded maximum number of threads.
 * New thread is allocated if there are no idle threads for the newly submitted
 * task and the maximum pool capacity is not reached.
 */
class ThreadPool {
public:
    ThreadPool(int maxthreads);

    virtual ~ThreadPool();

    /**
     * Submit new task to the pool
     * @return future for the submitted task
     */
    template<typename Fn, typename... Args, typename T = typename std::result_of<Fn(Args...)>::type>
    std::future<T> Submit(Fn &&fn, Args &&... args);

    /**
     * Joins all threads and removes them from pool, prevents new tasks.
     */
    void Shutdown();

protected:
    template<typename Fn, typename... Args, typename T = typename std::result_of<Fn(Args...)>::type>
    void Submit(std::shared_ptr<BlockingQueue<T>> resultSubmissionQueue, Fn &&fn, Args &&... args);

private:
    void TrySpawnThread();
    std::function<void() > GetNextTask();
    void Main();

    void Submit(std::function<void() > task);

    template<typename T>
    std::pair<std::function<void()>, std::future<T>> CreateNewTask(std::function<T() > call);

    template<typename T>
    std::function<void() > CreateNewTask(std::function<T() > call, std::shared_ptr<BlockingQueue<T>> resultSubmissionQueue);

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
    int idle_threads_;
};

ThreadPool::ThreadPool(int maxthreads = std::thread::hardware_concurrency())
: flag_terminate_threads_(false), maxthreads_(maxthreads), idle_threads_(0) {
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
    std::function < T() > call = std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...);
    std::pair < std::function<void()>, std::future < T>> task = CreateNewTask<T>(call);
    Submit(task.first);
    return std::move(task.second);
}

template<typename Fn, typename... Args, typename T>
void ThreadPool::Submit(std::shared_ptr<BlockingQueue<T>> resultSubmissionQueue, Fn &&fn, Args &&... args) {
    std::function < T() > call = std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...);
    std::function<void() > task = CreateNewTask<T>(call, resultSubmissionQueue);
    Submit(task);
}

void ThreadPool::Submit(std::function<void() > task) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (flag_terminate_threads_) return;
    
    if (idle_threads_ == 0)
        TrySpawnThread();

    tasks_.emplace(std::move(task));
    notify_.notify_one();
}

template<typename T>
std::pair<std::function<void()>, std::future<T>> ThreadPool::CreateNewTask(std::function<T() > call) {
    std::shared_ptr<std::promise < T>> promise = std::make_shared<std::promise < T >> ();
    std::future<T> future = promise->get_future();
    std::function<void() > callWrapper = [this, call, promise]() {
        try {
            SetPromiseValue(*promise, call);
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    };
    return std::make_pair(callWrapper, std::move(future));
}

template<typename T>
std::function<void() > ThreadPool::CreateNewTask(std::function<T() > call, std::shared_ptr<BlockingQueue<T>> resultSubmissionQueue) {
    std::shared_ptr<std::promise < T>> promise = std::make_shared<std::promise < T >> ();
    std::function<void() > callWrapper = [this, call, promise, resultSubmissionQueue]() {
        try {
            SetPromiseValue(*promise, call);
        } catch (...) {
            promise->set_exception(std::current_exception());
        }

        try {
            resultSubmissionQueue->put(promise->get_future());
        } catch (...) {
            std::cout << "ThreadPool failed to submit future " << std::endl;
            resultSubmissionQueue->put(std::future<T>());
        }
    };
    return callWrapper;
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

void ThreadPool::TrySpawnThread() {
    if (maxthreads_ > threads_.size()) {
        threads_.emplace_back(&ThreadPool::Main, this);
    }
}

std::function<void() > ThreadPool::GetNextTask() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!flag_terminate_threads_) {
        if (!tasks_.empty()) {
            if (idle_threads_ > 0) idle_threads_--;
            std::function<void() > task = tasks_.front();
            tasks_.pop();
            return task;
        }
        idle_threads_++;
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


