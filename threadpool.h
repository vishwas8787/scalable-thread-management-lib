#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include "tasks.h"

class ThreadPool {
public:
    ThreadPool(int numThreads);
    ~ThreadPool();

    void submit(std::function<void()> task);
    void shutdown();

    int getActiveThreads();
    int getQueuedTasks();
    int getCompletedTasks();

    int getCpuCompleted();
    int getIoCompleted();
    int getFibCompleted();
    void incrementTaskType(TaskType type);
    void submit(TaskType type, std::function<void()> task, int priority);

    long long getTotalExecutionTime();

    long long getCpuTime();
    long long getIoTime();
    long long getFibTime();

private:
    void worker();

    std::vector<std::thread> workers;
    // std::queue<std::function<void()>> tasks;

    std::mutex mtx;
    std::condition_variable cv;
    bool stop;

    std::atomic<int> activeThreads;
    std::atomic<int> completedTasks;

    std::atomic<int> cpuCompleted;
    std::atomic<int> ioCompleted;
    std::atomic<int> fibCompleted;
    // std::queue<std::pair<TaskType, std::function<void()>>> tasks;

    struct Task {
        TaskType type;
        std::function<void()> func;
        int priority;

        bool operator<(const Task& other) const {
            return priority < other.priority; // higher value = higher priority
        }
    };
    std::priority_queue<Task> tasks;

    std::atomic<long long> totalExecutionTime; // in ms

    std::atomic<long long> cpuTime;
    std::atomic<long long> ioTime;
    std::atomic<long long> fibTime;
};

#endif