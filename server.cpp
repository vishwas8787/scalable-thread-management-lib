#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include "threadpool.h"
#include "tasks.h"

#pragma comment(lib, "ws2_32.lib")

ThreadPool pool(4);

// 🔥 Build HTTP response
std::string makeResponse(const std::string& body, const std::string& type = "text/html") {
    return "HTTP/1.1 200 OK\r\nContent-Type: " + type + "\r\n\r\n" + body;
}

void handleClient(SOCKET clientSocket) {
    char buffer[2048] = {0};
    recv(clientSocket, buffer, sizeof(buffer), 0);

    std::string request(buffer);
    std::cout << "---- REQUEST ----\n" << request << "\n";

    std::string response;

    // 🔥 ROUTES

    // Home page
    if (request.find("GET / ") != std::string::npos) {
        std::string body =
            "<h1>ThreadPool Server</h1>"
            "<a href='/submit'>Submit Task</a><br>"
            "<a href='/stats'>View Stats</a><br>"
            "<a href='/cancel'>Cancel Tasks</a>";

        response = makeResponse(body);
    }

    // Submit task
    else if (request.find("GET /submit") != std::string::npos) {
        pool.submit(TaskType::CPU, []() { cpuTask(1); }, 2);
        response = makeResponse("<h2>Task Submitted</h2><a href='/'>Back</a>");
    }

    // Stats
    else if (request.find("GET /stats") != std::string::npos) {
        std::string body =
            "<h2>Stats</h2>"
            "Active: " + std::to_string(pool.getActiveThreads()) + "<br>"
            "Queued: " + std::to_string(pool.getQueuedTasks()) + "<br>"
            "Completed: " + std::to_string(pool.getCompletedTasks()) + "<br><br>"
            "<a href='/'>Back</a>";

        response = makeResponse(body);
    }

    // Cancel tasks
    else if (request.find("GET /cancel") != std::string::npos) {
        pool.cancelPendingTasks();
        response = makeResponse("<h2>Pending Tasks Cancelled</h2><a href='/'>Back</a>");
    }

    // Not found
    else {
        response = "HTTP/1.1 404 Not Found\r\n\r\n404 Not Found";
    }

    send(clientSocket, response.c_str(), response.size(), 0);
    closesocket(clientSocket);
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 5);

    std::cout << "Server running at http://localhost:8080\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        std::thread(handleClient, clientSocket).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}