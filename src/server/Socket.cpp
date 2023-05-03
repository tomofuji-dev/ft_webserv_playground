#include "Socket.hpp"
#include "Epoll.hpp"
#include "define.hpp"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

// ------------------------------------------------------------------
// 継承用のクラス

ASocket::ASocket(std::vector<VServer> config) : fd_(-1), config_(config) {}

ASocket::ASocket(const ASocket &src) : fd_(src.fd_) {}

ASocket::~ASocket() { close(fd_); }

ASocket &ASocket::operator=(const ASocket &rhs) {
  if (this != &rhs) {
    fd_ = rhs.fd_;
  }
  return *this;
}

int ASocket::GetFd() const { return fd_; }

void ASocket::SetFd(int fd) { fd_ = fd; }

int ASocket::SetNonBlocking() const {
  int flags = fcntl(fd_, F_GETFL, 0);
  if (flags == -1) {
    return FAILURE;
  }
  if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
    return FAILURE;
  }
  return SUCCESS;
}

// ------------------------------------------------------------------
// 通信用のソケット

ConnSocket::ConnSocket(std::vector<VServer> config) : ASocket(config) {}

ConnSocket::ConnSocket(const ConnSocket &src) : ASocket(src) {}

ConnSocket::~ConnSocket() {}

ConnSocket &ConnSocket::operator=(const ConnSocket &rhs) {
  if (this != &rhs) {
    fd_ = rhs.fd_;
  }
  return *this;
}

// 0: 引き続きsocketを利用 -1: socketを閉じる
int ConnSocket::OnReadable() {
  if (!recv_buffer_.ReadSocket(fd_)) {
    return FAILURE;
  }
  std::cout << recv_buffer_.GetString() << std::endl;
  request_.Parse(recv_buffer_);
  if (request_.GetStatus() == COMPLETE || request_.GetStatus() == ERROR) {
    Response response = ProcessRequest(request_, config_);
    send_buffer_.AddString(response.GetString());
    request_.Clear();
  }
  return SUCCESS;
}

// 0: 引き続きsocketを利用 -1: socketを閉じる
int ConnSocket::OnWritable() {
  send_buffer_.SendSocket(fd_);
  return SUCCESS;
}

int ConnSocket::ProcessSocket(Epoll *epoll_map, int event_fd, void *data) {
  // clientからの通信を処理
  uint32_t event_mask = *(static_cast<uint32_t *>(data));
  ConnSocket *client_socket =
      reinterpret_cast<ConnSocket *>(epoll_map->GetSocket(event_fd));
  if (client_socket == NULL) {
    return FAILURE;
  }
  if (event_mask & EPOLLIN) {
    // 受信(Todo: OnReadable(0))
    if (client_socket->OnReadable() == FAILURE) {
      epoll_map->Del(client_socket->GetFd());
      return FAILURE;
    }
  }
  if (event_mask & EPOLLPRI) {
    // 緊急メッセージ(Todo: OnReadable(MSG_OOB))
    if (client_socket->OnReadable() == FAILURE) {
      epoll_map->Del(client_socket->GetFd());
      return FAILURE;
    }
  }
  if (event_mask & EPOLLOUT) {
    // 送信
    if (client_socket->OnWritable() == FAILURE) {
      epoll_map->Del(client_socket->GetFd());
      return FAILURE;
    }
  }
  if (event_mask & EPOLLRDHUP) {
    // クライアントが切断
    shutdown(client_socket->GetFd(), SHUT_RD);
    shutdown(client_socket->GetFd(), SHUT_WR);
    epoll_map->Del(client_socket->GetFd());
  }
  if (event_mask & EPOLLERR || event_mask & EPOLLHUP) {
    // エラー
    epoll_map->Del(client_socket->GetFd());
  }
  return SUCCESS;
}

// ------------------------------------------------------------------
// listen用のソケット

ListenSocket::ListenSocket(std::vector<VServer> config) : ASocket(config) {}

ListenSocket::ListenSocket(const ListenSocket &src) : ASocket(src) {}

ListenSocket::~ListenSocket() {}

ListenSocket &ListenSocket::operator=(const ListenSocket &rhs) {
  if (this != &rhs) {
    fd_ = rhs.fd_;
  }
  return *this;
}

int ListenSocket::Create() {
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_ < 0) {
    return FAILURE;
  }
  return SetNonBlocking();
}

int ListenSocket::Passive() {
  struct sockaddr_in sockaddr;
  std::string ip = config_[0].listen_.listen_ip_;
  int port = config_[0].listen_.listen_port_;

  sockaddr.sin_family = AF_INET;
  sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());
  sockaddr.sin_port = htons(port);
  // Bind server socket to address
  if (bind(fd_, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1) {
    // error handling
    return FAILURE;
  }
  if (listen(fd_, SOMAXCONN) == -1) {
    return FAILURE;
  }
  return SUCCESS;
}

ConnSocket *ListenSocket::Accept() {
  ConnSocket *conn_socket = new ConnSocket(config_);
  struct sockaddr_in client_addr;
  socklen_t addrlen = sizeof(client_addr);

  conn_socket->SetFd(accept(fd_, (struct sockaddr *)&client_addr, &addrlen));
  if (conn_socket->GetFd() < 0) {
    delete conn_socket;
    return NULL;
  }
  if (conn_socket->SetNonBlocking() == FAILURE) {
    delete conn_socket;
    return NULL;
  }
  return conn_socket;
}

int ListenSocket::ProcessSocket(Epoll *epoll_map, int event_fd, void *data) {
  // 接続要求を処理
  (void)data;
  static uint32_t epoll_mask =
      EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLOUT | EPOLLET;
  ListenSocket *server_socket =
      reinterpret_cast<ListenSocket *>(epoll_map->GetSocket(event_fd));

  ConnSocket *client_socket = server_socket->Accept();
  if (client_socket == NULL ||
      epoll_map->Add(client_socket, epoll_mask) == FAILURE) {
    delete client_socket;
    return FAILURE;
  }
  return SUCCESS;
}
