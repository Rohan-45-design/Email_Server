#pragma once

#ifdef _WIN32

  #include <winsock2.h>
  #include <ws2tcpip.h>
  using socket_t = SOCKET;

  inline int close_socket(socket_t sock) {
    return closesocket(sock);
  }

#else

  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>

  using socket_t = int;
  constexpr int INVALID_SOCKET = -1;
  constexpr int SOCKET_ERROR   = -1;

  inline int close_socket(socket_t sock) {
    return close(sock);
  }

#endif
