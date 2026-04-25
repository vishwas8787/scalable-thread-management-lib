#include <iostream>
#include "threadpool.h"
#include <chrono>
#include <mutex>

std::mutex printMutex;

using namespace std;

void testTask(int id) {
    {
        std::lock_guard<std::mutex> lock(printMutex);
        std::cout << "Task " << id << " started\n";
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    {
        std::lock_guard<std::mutex> lock(printMutex);
        std::cout << "Task " << id << " finished\n";
    }
}

int main() {
    ThreadPool pool(3); // 3 threads

    for (int i = 0; i < 10; i++) {
        pool.submit([i]() {
            testTask(i);
        });
    }

    this_thread::sleep_for(chrono::seconds(5));

    cout << "Active threads: " << pool.getActiveThreads() << endl;
    cout << "Queued tasks: " << pool.getQueuedTasks() << endl;

    pool.shutdown();
}