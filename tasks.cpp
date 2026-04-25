// tasks.cpp

#include "tasks.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

// use the same mutex from main.cpp
extern std::mutex printMutex;

void cpuTask(int id) {
    {
        std::lock_guard<std::mutex> lock(printMutex);
        std::cout << "[CPU] Task " << id << " started\n";
    }

    // simulate CPU-heavy work
    for (int i = 0; i < 100000000; i++);

    {
        std::lock_guard<std::mutex> lock(printMutex);
        std::cout << "[CPU] Task " << id << " finished\n";
    }
}

void ioTask(int id) {
    {
        std::lock_guard<std::mutex> lock(printMutex);
        std::cout << "[IO] Task " << id << " started\n";
    }

    // simulate I/O delay
    std::this_thread::sleep_for(std::chrono::seconds(2));

    {
        std::lock_guard<std::mutex> lock(printMutex);
        std::cout << "[IO] Task " << id << " finished\n";
    }
}

int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

void fibTask(int id) {
    {
        std::lock_guard<std::mutex> lock(printMutex);
        std::cout << "[FIB] Task " << id << " started\n";
    }

    fib(30); // CPU-heavy recursive work

    {
        std::lock_guard<std::mutex> lock(printMutex);
        std::cout << "[FIB] Task " << id << " finished\n";
    }
}

// tasks.cpp