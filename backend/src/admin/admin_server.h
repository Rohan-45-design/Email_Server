#pragma once

#include <thread>
#include <atomic>

#include "core/platform_socket.h"

class AdminServer {
public:
    void start(int port = 8081);
    void stop();

private:
    void run(int port);

    std::atomic<bool> running_{false};
    std::thread thread_;

    socket_t listenSock_{INVALID_SOCKET};
};
