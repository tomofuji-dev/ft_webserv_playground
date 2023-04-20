#ifndef _EPOLL_HPP_
#define _EPOLL_HPP_

#include <map>
#include "Socket.hpp"

class Epoll {
	private:
		int epoll_fd_;
		std::map<int, ASocket*> fd_to_socket_;

	public:
		Epoll();
		Epoll(const Epoll& src);
		~Epoll();
		Epoll& operator=(const Epoll& rhs);

		int add(ASocket* socket, int option, uint32_t event_mask);
		int del(int fd);
		int wait(struct epoll_event* events, int maxevents, int timeout);
};

#endif