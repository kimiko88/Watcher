#include "cms/Socket.h" // Fixed include path
#include <iostream>

// Link with Ws2_32.lib on Windows
#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif

namespace cms {

Socket::Socket() : sock_(INVALID_SOCKET) {}

Socket::~Socket() { Close(); }

bool Socket::Initialize() {
#ifdef _WIN32
  WSADATA wsaData;
  int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (result != 0) {
    std::cerr << "WSAStartup failed: " << result << std::endl;
    return false;
  }
#endif
  return true;
}

void Socket::Cleanup() {
#ifdef _WIN32
  WSACleanup();
#endif
}

bool Socket::Connect(const std::string &host, int port) {
  if (sock_ != INVALID_SOCKET)
    Close();

  sock_ = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_ == INVALID_SOCKET)
    return false;

  sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);

#ifdef _WIN32
  inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);
#else
  inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);
#endif

  if (connect(sock_, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
    Close();
    return false;
  }

  return true;
}

bool Socket::Bind(int port) {
  if (sock_ != INVALID_SOCKET)
    Close();

  sock_ = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_ == INVALID_SOCKET)
    return false;

  // Allow reuse address
  int opt = 1;
  setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (bind(sock_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    Close();
    return false;
  }
  return true;
}

bool Socket::Listen() {
  if (listen(sock_, 5) < 0)
    return false;
  return true;
}

Socket *Socket::Accept() {
  if (sock_ == INVALID_SOCKET)
    return nullptr;

  sockaddr_in clientAddr;
#ifdef _WIN32
  int len = sizeof(clientAddr);
#else
  socklen_t len = sizeof(clientAddr);
#endif

  SOCKET clientSock = accept(sock_, (struct sockaddr *)&clientAddr, &len);
  if (clientSock == INVALID_SOCKET)
    return nullptr;

  Socket *newSocket = new Socket();
  newSocket->sock_ = clientSock;
  return newSocket;
}

bool Socket::Send(const std::string &data) {
  return Send(data.c_str(), data.length());
}

bool Socket::Send(const void *data, size_t len) {
  if (sock_ == INVALID_SOCKET)
    return false;

  const char *ptr = static_cast<const char *>(data);
  size_t totalSent = 0;

  while (totalSent < len) {
    int sent =
        send(sock_, ptr + totalSent, static_cast<int>(len - totalSent), 0);
    if (sent < 0)
      return false;
    totalSent += sent;
  }
  return true;
}

int Socket::Receive(void *buffer, size_t len) {
  if (sock_ == INVALID_SOCKET)
    return -1;
  return recv(sock_, static_cast<char *>(buffer), static_cast<int>(len), 0);
}

void Socket::Close() {
  if (sock_ != INVALID_SOCKET) {
#ifdef _WIN32
    closesocket(sock_);
#else
    close(sock_);
#endif
    sock_ = INVALID_SOCKET;
  }
}

bool Socket::IsValid() const { return sock_ != INVALID_SOCKET; }

} // namespace cms
