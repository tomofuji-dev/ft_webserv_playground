#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#include "Config.hpp"
#include "IOBuff.hpp"
#include <netinet/in.h>
#include <vector>

class Epoll; // 相互参照

// ------------------------------------------------------------------
// 継承用のクラス

class ASocket {
protected:
  int fd_;
  std::vector<VServer> config_;

public:
  ASocket(std::vector<VServer> config);
  ASocket(const ASocket &src);
  virtual ~ASocket();
  ASocket &operator=(const ASocket &rhs);

  int GetFd() const;
  void SetFd(int fd);
  int SetNonBlocking() const;
  virtual int ProcessSocket(Epoll *epoll_map, int event_fd, void *data) = 0;
};

// ------------------------------------------------------------------
// 通信用のソケット

class ConnSocket : public ASocket {
private:
  IOBuff recv_buffer_;
  IOBuff send_buffer_;

  void OnMessageReceived();
  bool IsMessageComplete() const;

public:
  ConnSocket(std::vector<VServer> config);
  ConnSocket(const ConnSocket &src);
  ~ConnSocket();
  ConnSocket &operator=(const ConnSocket &rhs);

  int OnReadable();
  int OnWritable();
  int ProcessSocket(Epoll *epoll_map, int event_fd, void *data);
};

// ------------------------------------------------------------------
// listen用のソケット

class ListenSocket : public ASocket {
private:
public:
  ListenSocket(std::vector<VServer> config);
  ListenSocket(const ListenSocket &src);
  ~ListenSocket();
  ListenSocket &operator=(const ListenSocket &rhs);

  int Create();
  int Passive();
  ConnSocket *Accept();
  int ProcessSocket(Epoll *epoll_map, int event_fd, void *data);
};

#endif