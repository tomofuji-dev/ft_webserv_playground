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

void ConnSocket::OnMessageReceived() {
  // request_.ContainBody(recv_buffer_);
  // std::vector<char> response = ProcessRequest(request_);
  // send_buffer_.insert(response.begin(), response.end());

  send_buffer_.insert(send_buffer_.end(), recv_buffer_.begin(),
                      recv_buffer_.end());
  recv_buffer_.erase(recv_buffer_.begin(), recv_buffer_.end());
}

bool ConnSocket::IsMessageComplete() const {
  return true;
  // if (request_.header_.IsComplete()) {
  //   return request_.IsComplete(recv_buffer_);
  // }
  // // CRLF*2がない場合はヘッダの受信が完了していない
  // if (!IsContain2CRLF(recv_buffer_)) {
  //   return false;
  // }
  // request_.header_.Parse(recv_buffer_);
  // return request_.IsComplete(recv_buffer_);
}

// 0: 引き続きsocketを利用 -1: socketを閉じる
int ConnSocket::OnReadable(int recv_flag) {
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;

  errno = 0;
  while (true) {
    bytes_read = recv(fd_, buffer, sizeof(buffer), recv_flag);
    if (bytes_read > 0) {
      recv_buffer_.insert(recv_buffer_.end(), buffer, buffer + bytes_read);

      // Check if the message is complete and notify the application
      if (IsMessageComplete()) {
        // エコーサーバーの場合、recv_bufferをsend_bufferにコピーする
        // HTTPの場合、返信内容をsend_bufferに作成して、recv_bufferを\r\n\r\n以降のものに書き換える
        OnMessageReceived();
      }
    } else if (bytes_read == 0) {
      // クライアントが接続を閉じた場合: 呼び出し側でfdを閉じる必要がある
      return FAILURE;
    } else if (errno != EAGAIN) {
      // read, recvが失敗した場合: 呼び出し側でfdを閉じる
      return FAILURE;
    } else {
      // 受信バッファが空の場合など、読み込み準備ができていない
      return SUCCESS;
    }
  }
  return SUCCESS;
}

// 0: 引き続きsocketを利用 -1: socketを閉じる
int ConnSocket::OnWritable() {
  ssize_t bytes_written;

  errno = 0;
  if (!send_buffer_.empty()) {
    bytes_written = send(fd_, send_buffer_.data(), send_buffer_.size(), 0);

    if (bytes_written > 0) {
      for (int i = 0; i < bytes_written; i++) {
        std::cout << send_buffer_[i];
      }
      std::cout << std::endl;
      send_buffer_.erase(send_buffer_.begin(),
                         send_buffer_.begin() + bytes_written);
    } else if (bytes_written < 0 && errno != EAGAIN) {
      // write, sendが失敗した場合: 呼び出し側でfdを閉じる
      return FAILURE;
    }
    // bytes_written == 0 || errno == EAGAIN || errno == EWOULDBLOCK ->
    // 正常な挙動として引き続きsocketを利用
  }
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
    // 受信
    if (client_socket->OnReadable(0) == FAILURE) {
      epoll_map->Del(client_socket->GetFd());
      return FAILURE;
    }
  }
  if (event_mask & EPOLLPRI) {
    // 緊急メッセージ
    if (client_socket->OnReadable(MSG_OOB) == FAILURE) {
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
