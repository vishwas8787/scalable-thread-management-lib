// threadpool.cpp

#include "threadpool.h"
#include <stdexcept>
#include "tasks.h"
#include <chrono>
ThreadPool::ThreadPool(int numThreads):stop(false),
    activeThreads(0),
    completedTasks(0),
    cpuCompleted(0),
    ioCompleted(0),
    fibCompleted(0),
    totalExecutionTime(0),
    cpuTime(0),
    ioTime(0),
    fibTime(0){
    
        for (int i = 0; i < numThreads; i++) {
        workers.emplace_back(&ThreadPool::worker, this);
        }
}
void ThreadPool::worker() {
    while (true) {
        TaskType type;
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return stop || !tasks.empty(); });
            if (stop && tasks.empty()) return;

            Task taskItem = tasks.top();   // 🔥 changed
            tasks.pop();

            type = taskItem.type;
            task = taskItem.func;
        }

        activeThreads++;

        auto start = std::chrono::high_resolution_clock::now();

        try {
            task();
        } catch (...) {
            // optional
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        activeThreads--;
        completedTasks++;

        totalExecutionTime += duration;

        switch (type) {
            case TaskType::CPU: 
                cpuCompleted++;
                cpuTime += duration;
                break;
            case TaskType::IO:  
                ioCompleted++;
                ioTime += duration;
                break;
            case TaskType::FIB: 
                fibCompleted++;
                fibTime += duration;
                break;
        }
    }
}

void ThreadPool::submit(TaskType type, std::function<void()> task, int priority) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (stop) throw std::runtime_error("Cannot submit to stopped pool");
        tasks.push({type, task, priority});
    }
    cv.notify_one();
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (stop) return;   // ✅ guard against double-shutdown
        stop = true;
    }
    cv.notify_all();
    for (auto& t : workers)
        if (t.joinable()) t.join();
}

void ThreadPool::cancelPendingTasks() {
    std::lock_guard<std::mutex> lock(mtx);

    while (!tasks.empty()) {
        tasks.pop();
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

int ThreadPool::getActiveThreads() {
    return activeThreads.load();
}

int ThreadPool::getCompletedTasks() {
    return completedTasks.load();
}

int ThreadPool::getQueuedTasks() {
    std::lock_guard<std::mutex> lock(mtx);
    return tasks.size();
}

int ThreadPool::getCpuCompleted() {
    return cpuCompleted.load();
}

int ThreadPool::getIoCompleted() {
    return ioCompleted.load();
}

int ThreadPool::getFibCompleted() {
    return fibCompleted.load();
}

long long ThreadPool::getTotalExecutionTime() {
    return totalExecutionTime.load();
}

long long ThreadPool::getCpuTime() {
    return cpuTime.load();
}

long long ThreadPool::getIoTime() {
    return ioTime.load();
}

long long ThreadPool::getFibTime() {
    return fibTime.load();
}

double ThreadPool::getAvgCpuTime() {
    int count = cpuCompleted.load();
    return count == 0 ? 0 : (double)cpuTime.load() / count;
}

double ThreadPool::getAvgIoTime() {
    int count = ioCompleted.load();
    return count == 0 ? 0 : (double)ioTime.load() / count;
}

double ThreadPool::getAvgFibTime() {
    int count = fibCompleted.load();
    return count == 0 ? 0 : (double)fibTime.load() / count;
}

double ThreadPool::getOverallAvgTime() {
    int count = completedTasks.load();
    return count == 0 ? 0 : (double)totalExecutionTime.load() / count;
}

double ThreadPool::getThroughput() {
    double totalTimeSec = totalExecutionTime.load() / 1000.0;
    if (totalTimeSec == 0) return 0;
    return completedTasks.load() / totalTimeSec;
}

int ThreadPool::getNumThreads() {
    return workers.size();
}

// threadpool.cpp
