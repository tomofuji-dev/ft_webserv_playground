#include "IOBuff.hpp"

#include <fstream>
#include <ios>
#include <sys/socket.h>

IOBuff::IOBuff() {}  // デフォルトコンストラクタ
IOBuff::~IOBuff() {} // デストラクタ

bool IOBuff::ReadSocket(int fd) {
  char buf[1024];
  ssize_t len;

  while (true) {
    len = recv(fd, buf, sizeof(buf), MSG_DONTWAIT);
    this->ss_ << std::string(buf, len);
    if (len < static_cast<ssize_t>(sizeof(buf))) {
      break;
    }
  }
  // len == 0: クライアントからの切断を意味する
  return len != 0;
}

bool IOBuff::GetLine(std::string &line) {
  std::getline(this->ss_, line);

  std::ios_base::iostate state = ss_.rdstate();
  if (state & std::ios_base::eofbit) {
    return false;
  } else if (state & std::ios_base::badbit) {
    throw std::runtime_error("IOBuff::GetLine() failed");
  }
  return true;
}

void IOBuff::ResetSeekg() { this->ss_.seekg(0); }

void IOBuff::ResetSeekp() { this->ss_.seekp(0); }

ssize_t IOBuff::FindString(const std::string &target) {
  ssize_t original_pos = static_cast<ssize_t>(this->ss_.tellg());
  ssize_t target_pos = -1;
  size_t target_index = 0;
  char ch;

  while (this->ss_.get(ch)) {
    if (ch == target[target_index]) {
      ++target_index;
      if (target_index == target.size()) {
        target_pos = static_cast<ssize_t>(this->ss_.tellg()) -
                     static_cast<ssize_t>(target.size());
        break;
      }
    } else {
      target_index = 0;
    }
  }

  // Reset the stream position to the original position
  this->ss_.clear();
  this->ss_.seekg(original_pos, std::ios_base::beg);

  return target_pos;
}

std::string IOBuff::GetAndErase(const size_t pos) {
  std::string str = this->ss_.str().substr(0, pos);
  this->ss_.str(this->ss_.str().erase(0, pos));
  this->ss_.seekg(0, std::ios::beg);
  this->ss_.seekp(0, std::ios::beg);
  return str;
}

void IOBuff::Erase() {
  this->ss_.str(this->ss_.str().erase(0, this->ss_.tellg()));
  this->ss_.seekg(0, std::ios::beg);
  this->ss_.seekp(0, std::ios::beg);
}

void IOBuff::Erase(size_t n) {
  this->ss_.str(this->ss_.str().erase(0, n));
  this->ss_.seekg(0, std::ios::beg);
  this->ss_.seekp(0, std::ios::beg);
}

size_t IOBuff::GetReadSize() {
  return this->ss_.tellg() == -1 ? 0 : static_cast<size_t>(this->ss_.tellg());
}

void IOBuff::AddString(const std::string &str) { this->ss_ << str; }

std::string IOBuff::GetString() { return this->ss_.str(); }

void IOBuff::ClearBuff() {
  this->ss_.str("");
  this->ss_.seekg(0, std::ios::beg);
  this->ss_.seekp(0, std::ios::beg);
}

bool IOBuff::SendSocket(const int fd) {
  size_t len = this->ss_.str().size();
  size_t send_len = send(fd, this->ss_.str().c_str(), len, MSG_DONTWAIT);
  this->Erase(send_len);
  return send_len == len;
}