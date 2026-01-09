#pragma once

/**
 * Cross-platform network initialization
 *
 * WHY REQUIRED:
 * - Winsock must be initialized once on Windows
 * - RAII ensures proper cleanup
 * - Centralized initialization prevents duplicate calls
 * - No-op on POSIX systems
 */
class NetworkInitializer {
public:
    NetworkInitializer();
    ~NetworkInitializer();
    
    // Prevent copying
    NetworkInitializer(const NetworkInitializer&) = delete;
    NetworkInitializer& operator=(const NetworkInitializer&) = delete;
    
private:
#ifdef _WIN32
    bool initialized_ = false;
#endif
};