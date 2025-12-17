#ifndef CMS_SOCKET_H
#define CMS_SOCKET_H

#include <cstdint>
#include <string>
#include <vector>


#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
using SOCKET = int;
#endif

namespace cms {

class Socket {
public:
  Socket();
  ~Socket();

  // Initialize networking (WSAStartup on Windows)
  static bool Initialize();
  static void Cleanup();

  // Client methods
  bool Connect(const std::string &host, int port);

  // Server methods
  bool Bind(int port);
  bool Listen();
  Socket *Accept();

  // I/O
  bool Send(const std::string &data);
  bool Send(const void *data, size_t len);

  // Receive with timeout (0 = blocking)
  // Returns number of bytes read, -1 on error, 0 on disconnect
  int Receive(void *buffer, size_t len);

  void Close();
  bool IsValid() const;

private:
  SOCKET sock_ = INVALID_SOCKET;
};

} // namespace cms

#endif // CMS_SOCKET_H
