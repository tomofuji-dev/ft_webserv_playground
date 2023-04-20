#include <vector>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include "Socket.hpp"
#include "define.hpp"

// ------------------------------------------------------------------
// 継承用のクラス

ASocket::ASocket(): fd_(-1) {
  memset(&sockaddr_, 0, sizeof(sockaddr_));
}

ASocket::ASocket(const ASocket& src): fd_(src.fd_) {}

ASocket::~ASocket() {
  close(fd_);
}

ASocket& ASocket::operator=(const ASocket& rhs) {
  if (this != &rhs) {
    fd_ = rhs.fd_;
  }
  return *this;
}

int ASocket::fd() const {
  return fd_;
}

int ASocket::set_non_blocking() const {
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

ConnSocket::ConnSocket(): ASocket() {}

ConnSocket::ConnSocket(const ConnSocket& src): ASocket(src) {}

ConnSocket::~ConnSocket() {}

ConnSocket& ConnSocket::operator=(const ConnSocket& rhs) {
  if (this != &rhs) {
    fd_ = rhs.fd_;
  }
  return *this; 
}

void ConnSocket::on_message_received() {
  send_buffer_.insert(send_buffer_.end(), recv_buffer_.begin(), recv_buffer_.end());
  recv_buffer_.erase(recv_buffer_.begin(), recv_buffer_.end());
}

bool ConnSocket::is_message_complete() const {
  return true;
}

// 0: 引き続きsocketを利用 -1: socketを閉じる
int ConnSocket::on_readable(int recv_flag) {
  char  buffer[1024];
  ssize_t bytes_read;

  errno = 0;
  while (true) {
  bytes_read = recv(fd_, buffer, sizeof(buffer), recv_flag);

    if (bytes_read > 0) {
      recv_buffer_.insert(recv_buffer_.end(), buffer, buffer + bytes_read);

      // Check if the message is complete and notify the application
      if (is_message_complete()) {
        // エコーサーバーの場合、recv_bufferをsend_bufferにコピーする
        // HTTPの場合、返信内容をsend_bufferに作成して、recv_bufferを\r\n\r\n以降のものに書き換える
        on_message_received();
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
  }
  return SUCCESS;
}

// 0: 引き続きsocketを利用 -1: socketを閉じる
int ConnSocket::on_writable() {
  ssize_t bytes_written;
  
  errno = 0;
  if (!send_buffer_.empty()) {
    bytes_written = send(fd_, send_buffer_.data(), send_buffer_.size(), 0);

    if (bytes_written > 0) {
      send_buffer_.erase(send_buffer_.begin(), send_buffer_.begin() + bytes_written);
    } else if (bytes_written < 0 && errno != EAGAIN) {
    // write, sendが失敗した場合: 呼び出し側でfdを閉じる
      return FAILURE;
    }
    // bytes_written == 0 || errno == EAGAIN || errno == EWOULDBLOCK -> 正常な挙動として引き続きsocketを利用
  }
  return SUCCESS;
}

// ------------------------------------------------------------------
// listen用のソケット

// fd_ == -1 チェックが必要
ListenSocket::ListenSocket(): ASocket() {}

ListenSocket::ListenSocket(const ListenSocket& src): ASocket(src) {}

ListenSocket::~ListenSocket() {}

ListenSocket& ListenSocket::operator=(const ListenSocket& rhs) {
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
  return set_non_blocking();
}

int ListenSocket::Passive(int port) {
  sockaddr_.sin_family = AF_INET;
  sockaddr_.sin_addr.s_addr = htonl(INADDR_ANY);
  sockaddr_.sin_port = htons(port);

  // Bind server socket to address
  if (bind(fd_, (struct sockaddr *)&sockaddr_, sizeof(sockaddr_)) == -1) {
    // error handling
    return FAILURE;
  }
  if (listen(fd_, SOMAXCONN) == -1) {
    return FAILURE;
  }
  return SUCCESS;
}

ConnSocket* ListenSocket::Accept() {
  ConnSocket* conn_socket = new ConnSocket();
  socklen_t addrlen = sizeof(conn_socket->sockaddr_);

  conn_socket->fd_ = accept(fd_, (struct sockaddr *)&conn_socket->sockaddr_, &addrlen);
  if (conn_socket->fd_ < 0) {
    delete conn_socket;
    return NULL;
  }
  if (conn_socket->set_non_blocking() == FAILURE) {
    delete conn_socket;
    return NULL;
  }
  return conn_socket;
}

