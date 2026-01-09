#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include "core/platform_socket.h"

class ServerContext;

class ImapServer {
public:
    ImapServer(ServerContext& ctx, int port);
    ~ImapServer();

    void start();
    void stop();

private:
    void run();

    ServerContext& ctx_;
    int port_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    socket_t listenSock_{INVALID_SOCKET};
    std::vector<std::thread> sessions_;
    std::mutex sessionsMutex_;
};
