#include "Epoll.hpp"
#include "Socket.hpp"
#include "define.hpp"
#include <sys/epoll.h>
#include <unistd.h>

int process_server_socket(Epoll *epoll_map, int event_fd);
int process_client_socket(Epoll *epoll_map, int event_fd, uint32_t event_mask);

int main() {
  ListenSocket *server_socket = new ListenSocket();
  Epoll *epoll_map = new Epoll();
  uint32_t epoll_mask = EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLOUT | EPOLLET;
  struct epoll_event events[MAX_EVENTS];

  if (server_socket->Create() == FAILURE ||
      server_socket->Passive(SERVER_PORT) == FAILURE ||
      epoll_map->Create() == FAILURE ||
      epoll_map->Add(server_socket, epoll_mask) == FAILURE) {
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
      int event_fd = events[i].data.fd;
      ASocket *event_socket = epoll_map->GetSocket(event_fd);
      uint32_t event_mask = events[i].events;
      event_socket->process_socket(epoll_map, event_fd, (void *)&event_mask);
    }
  }
  delete epoll_map;
  return SUCCESS;
}
