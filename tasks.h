#ifndef TASKS_H
#define TASKS_H

#include <mutex>

extern std::mutex printMutex;  // add this

enum class TaskType { CPU, IO, FIB };

void cpuTask(int id);
void ioTask(int id);
void fibTask(int id);

#endif