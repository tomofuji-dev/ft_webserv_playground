#include <vector>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include "Socket.hpp"

// ------------------------------------------------------------------
// 継承用のクラス

ASocket::ASocket(int fd): fd_(fd) {};

ASocket::ASocket(const ASocket& src): fd_(src.fd_) {};

ASocket::~ASocket() {
    close(fd_);
}

ASocket& ASocket::operator=(const ASocket& rhs) {
    if (this != &rhs) {
        fd_ = rhs.fd_;
    }
    return *this;
};

int ASocket::fd() const {
    return fd_;
};

// ------------------------------------------------------------------
// 通信用のソケット

ConnSocket::ConnSocket(int fd): ASocket(fd) {};

ConnSocket::ConnSocket(const ConnSocket& src): ASocket(src);

ConnSocket::~ConnSocket() {}

ConnSocket& ConnSocket::operator=(const ConnSocket& rhs) {
    if (this != &rhs) {
        fd_ = rhs.fd_;
    }
    return *this; 
};

void ConnSocket::on_message_received() {
    send_buffer_.insert(send_buffer_.end(), recv_buffer_.begin(), recv_buffer_.end());
    recv_buffer_.erase(recv_buffer_.begin(), recv_buffer_.end());
}

bool ConnSocket::is_message_complete() const {
    return true;
}

// 0: 引き続きsocketを利用 -1: socketを閉じる
int ConnSocket::on_readable(int recv_flag) {
    char    buffer[1024];
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
            }
        } else if (bytes_read == 0) {
            // クライアントが接続を閉じた場合: 呼び出し側でfdを閉じる必要がある
            return -1;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            // read, recvが失敗した場合: 呼び出し側でfdを閉じる
            return -1;
        } else {
            // 受信バッファが空の場合など、読み込み準備ができていない
            return 0;
        }
    }
    return 0;
}

// 0: 引き続きsocketを利用 -1: socketを閉じる
int ConnSocket::on_writable() {
    ssize_t bytes_written;
    
    errno = 0;
    if (!send_buffer_.empty()) {
        bytes_written = send(fd_, send_buffer_.data(), send_buffer_.size());
        if (bytes_written > 0) {
            send_buffer_.erase(send_buffer_.begin(), send_buffer_.begin() + bytes_written);
        } else if (bytes_written != 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            // write, sendが失敗した場合: 呼び出し側でfdを閉じる
            return -1;
        }
        // bytes_written == 0 || errno == EAGAIN || errno == EWOULDBLOCK -> 正常な挙動として引き続きsocketを利用
    }
    return 0;
}

// ------------------------------------------------------------------
// listen用のソケット


