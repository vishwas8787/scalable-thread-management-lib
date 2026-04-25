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

    for (int i = 0; i < 30; i++) {
        TaskType type;

        if      (i % 3 == 0) type = TaskType::CPU;
        else if (i % 3 == 1) type = TaskType::IO;
        else                  type = TaskType::FIB;

        try {
            int priority;

            if (type == TaskType::CPU) priority = 3;
            else if (type == TaskType::IO) priority = 2;
            else priority = 1;

            pool.submit(type, [i, type, priority]() {
                {
                    std::lock_guard<std::mutex> lock(printMutex);
                    std::cout << "Task " << i 
                            << " (priority " << priority << ") started\n";
                }
                switch (type) {
                    case TaskType::CPU: cpuTask(i); break;
                    case TaskType::IO:  ioTask(i);  break;
                    case TaskType::FIB: fibTask(i); break;
                }
            }, priority);
        } catch (const std::exception& e) {
            cout << "Submit failed: " << e.what() << "\n";
        }
    }

    // monitor loop
    for (int i = 0; i < 15; i++) {
        this_thread::sleep_for(chrono::seconds(1));

        if (i == 2) {
            {
                lock_guard<mutex> lock(printMutex);
                cout << "\n>>> Cancelling pending tasks...\n";
            }
            pool.cancelPendingTasks();
        }

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
        cout << "  Total Exec Time: " << pool.getTotalExecutionTime() << " ms\n";
        cout << "  CPU Time: " << pool.getCpuTime() << " ms\n";
        cout << "  IO Time: " << pool.getIoTime() << " ms\n";
        cout << "  FIB Time: " << pool.getFibTime() << " ms\n";
        cout << "  Avg CPU Time: " << pool.getAvgCpuTime() << " ms\n";
        cout << "  Avg IO Time:  " << pool.getAvgIoTime()  << " ms\n";
        cout << "  Avg FIB Time: " << pool.getAvgFibTime() << " ms\n";
        cout << "  Overall Avg:  " << pool.getOverallAvgTime() << " ms\n";
        cout << "  Throughput:   " << pool.getThroughput() << " tasks/sec\n";
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
        cout << "  Total Exec Time: " << pool.getTotalExecutionTime() << " ms\n";
        cout << "  CPU Time: " << pool.getCpuTime() << " ms\n";
        cout << "  IO Time: " << pool.getIoTime() << " ms\n";
        cout << "  FIB Time: " << pool.getFibTime() << " ms\n";
        cout << "  Avg CPU Time: " << pool.getAvgCpuTime() << " ms\n";
        cout << "  Avg IO Time:  " << pool.getAvgIoTime()  << " ms\n";
        cout << "  Avg FIB Time: " << pool.getAvgFibTime() << " ms\n";
        cout << "  Overall Avg:  " << pool.getOverallAvgTime() << " ms\n";
        cout << "  Throughput:   " << pool.getThroughput() << " tasks/sec\n";
    }

    return 0;
}