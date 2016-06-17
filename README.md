# Completion Thread Pool

Based on regular thread pool, this class provides means to submit tasks and retrieve their results in order of the completion.

  - Task is submitted to pool as a function with a number of arguments.
  - Tasks results are delivered with a standard std::future<T> objects.

### Version
1.0

### Usage

Create a task to be submitted to the pool:

```cpp
int f(int sleep_time) {
    sleep(sleep_time);
    return sleep_time;
}
```

Create CompletionThreadPool instance with designated task result type:

```cpp
CompletionThreadPool<int> completionThreadPool;
```

Submit any number of tasks for execution (they will be enqueued for execution immediately):

```cpp 
completionThreadPool.Submit(f, 5);
completionThreadPool.Submit(f, 1);
 ```
 
 Block waiting for the results in order of their readiness:
 
 ```cpp 
std::future<int> firstCompleted = completionThreadPool.Take();
std::future<int> secondCompleted = completionThreadPool.Take();
 ```
