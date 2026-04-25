#include "threadpool.h"
#include <stdexcept>
#include "tasks.h"

ThreadPool::ThreadPool(int numThreads):stop(false),
    activeThreads(0),
    completedTasks(0),
    cpuCompleted(0),
    ioCompleted(0),
    fibCompleted(0){
    
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

            type = tasks.front().first;
            task = std::move(tasks.front().second);
            tasks.pop();
        }
        // activeThreads++ AFTER lock release — cleaner, no deadlock risk
        activeThreads++;
        try {
            task();
        } catch (...) {
            // optional: log error
        }
        activeThreads--;
        completedTasks++;

        // 👇 ADD THIS
        switch (type) {
            case TaskType::CPU: cpuCompleted++; break;
            case TaskType::IO:  ioCompleted++;  break;
            case TaskType::FIB: fibCompleted++; break;
        }
    }
}

void ThreadPool::submit(TaskType type, std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (stop) throw std::runtime_error("Cannot submit to stopped pool");
        tasks.push({type, task});
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

void ThreadPool::incrementTaskType(TaskType type) {
    switch (type) {
        case TaskType::CPU:
            cpuCompleted++;
            break;
        case TaskType::IO:
            ioCompleted++;
            break;
        case TaskType::FIB:
            fibCompleted++;
            break;
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