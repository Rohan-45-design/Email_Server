#pragma once

#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>

#include "core/platform_socket.h"

class ServerContext; // forward declaration

class SmtpServer {
public:
    SmtpServer(ServerContext& ctx, int port);
    ~SmtpServer();

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
    std::vector<socket_t> clientSockets_;
    std::mutex clientsMutex_;

    std::atomic<int> recentFailures_{0};
    std::atomic<std::chrono::steady_clock::time_point> lastFailureTime_;
    static constexpr int FAILURE_THRESHOLD = 10;
    static constexpr int CIRCUIT_BREAKER_TIMEOUT_MS = 30000;

    bool isCircuitBreakerTripped() const;
    void resetCircuitBreakerIfExpired();
    void recordSessionFailure();
    void cleanupFinishedThreads();
};
