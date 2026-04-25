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

void heavyTask(int id) {
    {
        std::lock_guard<std::mutex> lock(printMutex);
        std::cout << "Task " << id << " started\n";
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    {
        std::lock_guard<std::mutex> lock(printMutex);
        std::cout << "Task " << id << " finished\n";
    }
}

int main() {
    try {
        ThreadPool pool(4);

        for (int i = 0; i < 10; i++) {
            pool.submit([i]() {
                heavyTask(i);
            });
        }

        for (int i = 0; i < 5; i++) {
            this_thread::sleep_for(chrono::seconds(1));

            int active = pool.getActiveThreads();
            int queued = pool.getQueuedTasks();
            int completed = pool.getCompletedTasks();
            
            {
                std::lock_guard<std::mutex> lock(printMutex);
                cout << "\n--- STATS ---\n";
                cout << "Active: " << active << endl;
                cout << "Queued: " << queued << endl;
                cout << "Completed: " << completed << endl;
            }
        }

        pool.shutdown();
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}