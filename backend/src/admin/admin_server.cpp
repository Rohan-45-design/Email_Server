#include "admin/admin_server.h"
#include "admin/admin_routes.h"
#include "core/logger.h"

#include "core/platform_socket.h"

#include <string>
#include <cerrno>
#include <stdexcept>

#ifndef _WIN32
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

void AdminServer::start(int port) {
    running_ = true;
    thread_ = std::thread(&AdminServer::run, this, port);
}

void AdminServer::stop() {
    running_ = false;

    if (listenSock_ != INVALID_SOCKET) {
        shutdown(listenSock_, SHUT_RDWR);
        close_socket(listenSock_);
        listenSock_ = INVALID_SOCKET;
    }

    if (thread_.joinable())
        thread_.join();
}

void AdminServer::run(int port) {

#ifdef _WIN32
    // Network initialization is now handled centrally
#endif

    listenSock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock_ == INVALID_SOCKET) {
#ifdef _WIN32
        int error = WSAGetLastError();
#else
        int error = errno;
#endif
        Logger::instance().log(LogLevel::Error,
            "AdminServer: socket() failed with error " + std::to_string(error));
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    int opt = 1;
    setsockopt(listenSock_, SOL_SOCKET, SO_REUSEADDR,
               (char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSock_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close_socket(listenSock_);
        listenSock_ = INVALID_SOCKET;
        Logger::instance().log(LogLevel::Error,
            "AdminServer: bind() failed on port " + std::to_string(port) +
            " with error " + std::to_string(errno));
        return;
    }

    if (listen(listenSock_, 5) < 0) {
#ifdef _WIN32
        int error = WSAGetLastError();
        closesocket(listenSock_);
        WSACleanup();
#else
        int error = errno;
        close(listenSock_);
#endif
        listenSock_ = INVALID_SOCKET;
        Logger::instance().log(LogLevel::Error,
            "AdminServer: listen() failed with error " + std::to_string(error));
        return;
    }

    Logger::instance().log(LogLevel::Info,
        "Admin API listening on port " + std::to_string(port));

    while (running_) {
        socket_t c = accept(listenSock_, nullptr, nullptr);
        if (c == INVALID_SOCKET) {
            if (running_) {
#ifdef _WIN32
                int error = WSAGetLastError();
                if (error != WSAEINTR)
#else
                int error = errno;
                if (error != EINTR)
#endif
                    Logger::instance().log(LogLevel::Warn,
                        "AdminServer: accept() failed with error " +
                        std::to_string(error));
            }
            continue;
        }

        try {
            std::string response = AdminRoutes::handleRequest(c);
            send(c, response.c_str(),
                 static_cast<int>(response.size()), 0);
        } catch (const std::exception& ex) {
            Logger::instance().log(LogLevel::Error,
                "AdminServer: Exception handling request: " +
                std::string(ex.what()));
        }

#ifdef _WIN32
        closesocket(c);
#else
        close(c);
#endif
    }

#ifdef _WIN32
    if (listenSock_ != INVALID_SOCKET)
        closesocket(listenSock_);
#else
    if (listenSock_ != INVALID_SOCKET)
        close(listenSock_);
#endif
}
