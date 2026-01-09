#include "network_init.h"
#include "logger.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

NetworkInitializer::NetworkInitializer() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        Logger::instance().log(LogLevel::Error, 
            "Network initialization failed: WSAStartup error " + std::to_string(result));
        throw std::runtime_error("WSAStartup failed");
    }
    initialized_ = true;
    Logger::instance().log(LogLevel::Info, "Network initialized (Windows)");
#endif
}

NetworkInitializer::~NetworkInitializer() {
#ifdef _WIN32
    if (initialized_) {
        WSACleanup();
        Logger::instance().log(LogLevel::Info, "Network cleanup completed (Windows)");
    }
#endif
}