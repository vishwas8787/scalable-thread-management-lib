#include <iostream>
#include <chrono>
#include <mutex>
#include "threadpool.h"
#include "tasks.h"
#include <thread>

std::mutex printMutex;

using namespace std;

int main() {
    ThreadPool pool(4);

    cout << "=== Thread Pool Started (4 threads) ===\n\n";

    for (int i = 0; i < 10; i++) {
        TaskType type;

        if      (i % 3 == 0) type = TaskType::CPU;
        else if (i % 3 == 1) type = TaskType::IO;
        else                  type = TaskType::FIB;

        try {
            pool.submit(type, [i, type]() {
                switch (type) {
                    case TaskType::CPU: cpuTask(i); break;
                    case TaskType::IO:  ioTask(i);  break;
                    case TaskType::FIB: fibTask(i); break;
                }
            });
        } catch (const std::exception& e) {
            cout << "Submit failed: " << e.what() << "\n";
        }
    }

    // monitor loop
    for (int i = 0; i < 6; i++) {
        this_thread::sleep_for(chrono::seconds(1));

        int active = pool.getActiveThreads();
        int queued = pool.getQueuedTasks();
        int completed = pool.getCompletedTasks();

        lock_guard<mutex> lock(printMutex);
        cout << "\n--- STATS (t+" << (i+1) << "s) ---\n";
        cout << "  Active:    " << active << "\n";
        cout << "  Queued:    " << queued << "\n";
        cout << "  Completed: " << completed << "\n";
        cout << "  CPU Completed: " << pool.getCpuCompleted() << "\n";
        cout << "  IO Completed:  " << pool.getIoCompleted()  << "\n";
        cout << "  FIB Completed: " << pool.getFibCompleted() << "\n";
    }

    pool.shutdown();

    {
        lock_guard<mutex> lock(printMutex);
        cout << "\n=== FINAL STATS ===\n";
        cout << "  Active:    " << pool.getActiveThreads() << "\n";
        cout << "  Queued:    " << pool.getQueuedTasks()   << "\n";
        cout << "  Completed: " << pool.getCompletedTasks()<< "\n";
        cout << "  CPU Completed: " << pool.getCpuCompleted() << "\n";
        cout << "  IO Completed:  " << pool.getIoCompleted()  << "\n";
        cout << "  FIB Completed: " << pool.getFibCompleted() << "\n";
    }

    return 0;
}