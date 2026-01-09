#include "virus_scanner.h"
#include "core/logger.h"

#include "core/platform_socket.h"

#include <chrono>
#include <string>
#include <cerrno>
#include <cstring>
#include <stdexcept>

#ifndef _WIN32
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

VirusScanResult VirusScanner::scan(const std::string& raw) {
    auto scan_start = std::chrono::steady_clock::now();

    VirusScanResult r;

    const int SCAN_TIMEOUT_MS = 30000; // 30 seconds max
    auto timeout_point = scan_start + std::chrono::milliseconds(SCAN_TIMEOUT_MS);

#ifdef _WIN32
    // Network initialization is now handled centrally
#endif

    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        r.unavailable = true;
        Logger::instance().log(LogLevel::Warn,
            "VirusScanner: Socket creation failed");
        return r;
    }

    /* ---------- Socket timeout ---------- */
#ifdef _WIN32
    DWORD timeout = 10000; // 10 seconds
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
               (char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
               (char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
               &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
               &tv, sizeof(tv));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(3310);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        r.unavailable = true;
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        Logger::instance().log(LogLevel::Warn,
            "VirusScanner: Connection to antivirus daemon failed");
        return r;
    }

    try {
        // INSTREAM command must include a null terminator
        const char cmd[] = "zINSTREAM\0";
        if (send(sock, cmd, sizeof(cmd), 0) < 0)
            throw std::runtime_error("Send command failed");

        uint32_t len = htonl(static_cast<uint32_t>(raw.size()));
        if (send(sock, (char*)&len, sizeof(len), 0) < 0)
            throw std::runtime_error("Send length failed");

        if (send(sock, raw.data(), raw.size(), 0) < 0)
            throw std::runtime_error("Send data failed");

        uint32_t zero = 0;
        if (send(sock, (char*)&zero, sizeof(zero), 0) < 0)
            throw std::runtime_error("Send terminator failed");

        char buf[1024];
        int received = recv(sock, buf, sizeof(buf), 0);
        if (received <= 0)
            throw std::runtime_error("Receive response failed");

        std::string reply(buf, received);

        if (reply.find("FOUND") != std::string::npos) {
            r.infected = true;
            r.virusName = reply;
        } else if (reply.find("OK") != std::string::npos) {
            r.clean = true;
        } else {
            r.unavailable = true;
        }

    } catch (const std::exception& ex) {
        r.unavailable = true;
        Logger::instance().log(LogLevel::Warn,
            "VirusScanner: Scan failed: " + std::string(ex.what()));
    }

#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif

    auto scan_end = std::chrono::steady_clock::now();
    auto duration_ms =
        std::chrono::duration<double, std::milli>(scan_end - scan_start).count();

    if (std::chrono::steady_clock::now() > timeout_point) {
        Logger::instance().log(LogLevel::Warn,
            "VirusScanner: Scan timed out");
        r.unavailable = true;
    }

    Logger::instance().observe_virus_scan(duration_ms);

    return r;
}
