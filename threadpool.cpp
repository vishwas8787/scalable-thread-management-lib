#include "threadpool.h"

ThreadPool::ThreadPool(int numThreads) : stop(false), activeThreads(0) {
    for (int i = 0; i < numThreads; i++) {
        workers.emplace_back(&ThreadPool::worker, this);
    }
}

void ThreadPool::worker() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] {
                return stop || !tasks.empty();
            });

            if (stop && tasks.empty())
                return;

            task = std::move(tasks.front());
            tasks.pop();
            activeThreads++;
        }

        task(); // execute

        activeThreads--;
    }
}

void ThreadPool::submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        tasks.push(task);
    }
    cv.notify_one();
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        stop = true;
    }

    cv.notify_all();

    for (auto &t : workers) {
        if (t.joinable())
            t.join();
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

int ThreadPool::getActiveThreads() {
    return activeThreads.load();
}

int ThreadPool::getQueuedTasks() {
    std::lock_guard<std::mutex> lock(mtx);
    return tasks.size();
}