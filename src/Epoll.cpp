#include "Epoll.hpp"
#include <sys/epoll.h>

Epoll::Epoll() {
	epoll_fd_ = epoll_create(1);
}

Epoll::Epoll(const Epoll& src) {
	*this = src;
}

Epoll::~Epoll() {
	close(epoll_fd_);

	map<int, ASocket*>::iterator it = fd_to_socket_.begin();
	while (it != fd_to_socket_.end()) {
		delete it->second;
		fd_to_socket_.erase(it++);
	}
}

Epoll& Epoll::operator=(const Epoll& rhs) {
	if (this != &rhs) {
		epoll_fd_ = rhs.epoll_fd_;
		fd_to_socket_ = rhs.fd_to_socket_;
	}
	return *this;
}

int Epoll::add(ASocket* socket, int option, uint32_t event_mask) {
	struct epoll_event ev;

	ev.events = option;
  ev.data.fd = socket->fd();
	if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket->fd(), &ev) == -1) {
		return FAILURE;
	}
	if (fd_to_socket_.find(socket->fd()) != fd_to_socket_.end()) {
		delete fd_to_socket_[socket->fd()];
	}
	fd_to_socket_[socket->fd()] = socket;
	return SUCCESS;
}

int Epoll::del(int fd) {
	if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL) == -1) {
		return FAILURE;
	}
	if (fd_to_socket_.find(fd) != fd_to_socket_.end()) {
		delete fd_to_socket_[fd];
		fd_to_socket_.erase(fd);
	}
	return SUCCESS;
}

int Epoll::wait(struct epoll_event* events, int maxevents, int timeout) {
	return epoll_wait(epoll_fd_, events, maxevents, timeout);
}
