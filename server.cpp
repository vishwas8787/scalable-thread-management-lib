// server.cpp

#include "threadpool.h"
#include "tasks.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <mutex>

// Windows headers
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

std::mutex printMutex;

ThreadPool* globalPool = nullptr;

// ── Build JSON stats response ─────────────────────────────────────
std::string getStatsJson() {
    std::ostringstream json;
    json << "{"
         << "\"active\":"      << globalPool->getActiveThreads()     << ","
         << "\"queued\":"      << globalPool->getQueuedTasks()       << ","
         << "\"completed\":"   << globalPool->getCompletedTasks()    << ","
         << "\"numThreads\":"  << globalPool->getNumThreads()        << ","
         << "\"cpu\":"         << globalPool->getCpuCompleted()      << ","
         << "\"io\":"          << globalPool->getIoCompleted()       << ","
         << "\"fib\":"         << globalPool->getFibCompleted()      << ","
         << "\"totalTime\":"   << globalPool->getTotalExecutionTime()<< ","
         << "\"cpuTime\":"     << globalPool->getCpuTime()           << ","
         << "\"ioTime\":"      << globalPool->getIoTime()            << ","
         << "\"fibTime\":"     << globalPool->getFibTime()           << ","
         << "\"avgCpu\":"      << globalPool->getAvgCpuTime()        << ","
         << "\"avgIo\":"       << globalPool->getAvgIoTime()         << ","
         << "\"avgFib\":"      << globalPool->getAvgFibTime()        << ","
         << "\"overallAvg\":"  << globalPool->getOverallAvgTime()    << ","
         << "\"throughput\":"  << globalPool->getThroughput()
         << "}";
    return json.str();
}

// ── Send HTTP response ────────────────────────────────────────────
void sendResponse(SOCKET client, const std::string& body,
                  const std::string& contentType = "application/json") {
    std::ostringstream resp;
    resp << "HTTP/1.1 200 OK\r\n"
         << "Content-Type: " << contentType << "\r\n"
         << "Access-Control-Allow-Origin: *\r\n"
         << "Content-Length: " << body.size() << "\r\n"
         << "Connection: close\r\n\r\n"
         << body;
    std::string r = resp.str();
    send(client, r.c_str(), (int)r.size(), 0);
}

// ── Parse path from HTTP request line ────────────────────────────
std::string parsePath(const std::string& request) {
    size_t start = request.find(' ');
    if (start == std::string::npos) return "/";
    size_t end = request.find(' ', start + 1);
    if (end == std::string::npos) return "/";
    return request.substr(start + 1, end - start - 1);
}

// ── Parse query param value ───────────────────────────────────────
std::string parseParam(const std::string& path, const std::string& key) {
    size_t pos = path.find(key + "=");
    if (pos == std::string::npos) return "";
    size_t start = pos + key.size() + 1;
    size_t end = path.find('&', start);
    return path.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

int main() {
    // Init winsock
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    globalPool = new ThreadPool(4);
    std::cout << "=== Thread Pool GUI Server ===\n";
    std::cout << "Open dashboard.html in your browser\n";
    std::cout << "Server running on http://localhost:8080\n\n";

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);

    // Allow port reuse
    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(8080);

    bind(server, (sockaddr*)&addr, sizeof(addr));
    listen(server, 10);

    while (true) {
        SOCKET client = accept(server, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;

        char buf[4096] = {};
        recv(client, buf, sizeof(buf) - 1, 0);
        std::string request(buf);
        std::string path = parsePath(request);

        // ── GET /stats ──────────────────────────────────────────
        if (path == "/stats") {
            sendResponse(client, getStatsJson());
        }

        // ── POST /submit?count=N ────────────────────────────────
        else if (path.rfind("/submit", 0) == 0) {
            std::string countStr = parseParam(path, "count");
            int count = countStr.empty() ? 5 : std::stoi(countStr);

            for (int i = 0; i < count; i++) {
                TaskType type;
                int priority;
                if      (i % 3 == 0) { type = TaskType::CPU; priority = 3; }
                else if (i % 3 == 1) { type = TaskType::IO;  priority = 2; }
                else                  { type = TaskType::FIB; priority = 1; }

                try {
                    globalPool->submit(type, [i, type]() {
                        switch (type) {
                            case TaskType::CPU: cpuTask(i); break;
                            case TaskType::IO:  ioTask(i);  break;
                            case TaskType::FIB: fibTask(i); break;
                        }
                    }, priority);
                } catch (...) {}
            }
            sendResponse(client, "{\"status\":\"submitted\",\"count\":" + std::to_string(count) + "}");
        }

        // ── POST /cancel ────────────────────────────────────────
        else if (path.rfind("/cancel", 0) == 0) {
            globalPool->cancelPendingTasks();
            sendResponse(client, "{\"status\":\"cancelled\"}");
        }

        // ── POST /reset?threads=N ───────────────────────────────
        else if (path.rfind("/reset", 0) == 0) {
            std::string tStr = parseParam(path, "threads");
            int threads = tStr.empty() ? 4 : std::stoi(tStr);
            if (threads < 1)  threads = 1;
            if (threads > 256) threads = 256;
            globalPool->shutdown();
            delete globalPool;
            globalPool = new ThreadPool(threads);
            std::cout << "Pool reset: now running " << threads << " thread(s)\n";
            sendResponse(client, "{\"status\":\"reset\",\"threads\":" + std::to_string(threads) + "}");
        }

        else {
            std::string body = "Not Found";
            std::ostringstream resp;
            resp << "HTTP/1.1 404 Not Found\r\nContent-Length: "
                 << body.size() << "\r\n\r\n" << body;
            std::string r = resp.str();
            send(client, r.c_str(), (int)r.size(), 0);
        }

        closesocket(client);
    }

    delete globalPool;
    WSACleanup();
    return 0;
}


// server.cpp