#include "Epoll.hpp"
#include "Socket.hpp"
#include "define.hpp"
#include <unistd.h>
#include <sys/epoll.h>

int process_server_socket(Epoll* epoll_map, ListenSocket* server_socket, uint32_t epoll_mask);
int process_client_socket(Epoll* epoll_map, int event_fd, uint32_t event_mask);

int main() {
	ListenSocket* 			server_socket	= new ListenSocket();
	Epoll* 							epoll_map = new Epoll();
	uint32_t 						epoll_mask = EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLOUT | EPOLLET;
	struct epoll_event	events[MAX_EVENTS];
	
	if (server_socket->Create() == FAILURE
			|| server_socket->Passive(SERVER_PORT) == FAILURE
			|| epoll_map->Create() == FAILURE
			|| epoll_map->Add(server_socket, epoll_mask) == FAILURE) {
		delete server_socket;
		delete epoll_map;
		return FAILURE;
	}
	while (true) {
		int nfds = epoll_map->Wait(events, MAX_EVENTS, -1);
		if (nfds == -1) {
			delete epoll_map;
			return FAILURE;	
		}
		for (int i = 0; i < nfds; i++) {
			int				event_fd = events[i].data.fd;
			uint32_t	event_mask = events[i].events;
			if (event_fd == server_socket->GetFd()) {
				process_server_socket(epoll_map, server_socket, epoll_mask);
			} else {
				process_client_socket(epoll_map, event_fd, event_mask);
			}
		}
	}
	delete epoll_map;
	return SUCCESS;
}

int process_server_socket(Epoll* epoll_map, ListenSocket* server_socket, uint32_t epoll_mask) {
	// 接続要求を処理
	ConnSocket* client_socket = server_socket->Accept();
	if (client_socket == NULL
			|| epoll_map->Add(client_socket, epoll_mask) == FAILURE) {
		delete client_socket;
		return FAILURE;
	}
	return SUCCESS;
}

int process_client_socket(Epoll* epoll_map, int event_fd, uint32_t event_mask) {
	// clientからの通信を処理
	ConnSocket* client_socket = static_cast<ConnSocket* >(epoll_map->GetSocket(event_fd));
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
