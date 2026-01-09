#pragma once

#include <string>
#include "core/platform_socket.h"

class AdminRoutes {
public:
    static std::string handleRequest(socket_t socket);
};
