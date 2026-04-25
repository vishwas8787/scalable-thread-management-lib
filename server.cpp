#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <fstream>
#include <sstream>

#include "threadpool.h"
#include "tasks.h"

#pragma comment(lib, "ws2_32.lib")

ThreadPool pool(4);

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string makeResponse(const std::string& body, const std::string& type) {
    return "HTTP/1.1 200 OK\r\nContent-Type: " + type + "\r\n\r\n" + body;
}

void handleClient(SOCKET clientSocket) {
    char buffer[2048] = {0};
    recv(clientSocket, buffer, sizeof(buffer), 0);

    std::string request(buffer);
    std::string response;

    // 🔹 Static files
    if (request.find("GET / ") != std::string::npos) {
        response = makeResponse(readFile("index.html"), "text/html");
    }
    else if (request.find("GET /style.css") != std::string::npos) {
        response = makeResponse(readFile("style.css"), "text/css");
    }
    else if (request.find("GET /script.js") != std::string::npos) {
        response = makeResponse(readFile("script.js"), "application/javascript");
    }

    // 🔹 API
    else if (request.find("GET /submit") != std::string::npos) {
        static int i = 0;
        int id=i;

        TaskType type;
        if (i % 3 == 0) type = TaskType::CPU;
        else if (i % 3 == 1) type = TaskType::IO;
        else type = TaskType::FIB;

        pool.submit(type, [id, type]() {
            if (type == TaskType::CPU) cpuTask(i);
            else if (type == TaskType::IO) ioTask(i);
            else fibTask(i);
        },2);

        i++;
        response = makeResponse("OK", "text/plain");
    }
    else if (request.find("GET /stats") != std::string::npos) {
        std::string body =
            "Active: " + std::to_string(pool.getActiveThreads()) + "\n" +
            "Queued: " + std::to_string(pool.getQueuedTasks()) + "\n" +
            "Completed: " + std::to_string(pool.getCompletedTasks());

        response = makeResponse(body, "text/plain");
    }
    else if (request.find("GET /cancel") != std::string::npos) {
        pool.cancelPendingTasks();
        response = makeResponse("Cancelled", "text/plain");
    }
    else {
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
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
}