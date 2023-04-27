#ifndef SERVER_HPP
#define SERVER_HPP

#include "Config.hpp"
#include "Epoll.hpp"

void RegisterListenSocket(Epoll &epoll, const Config &config);
void ServerLoop(Epoll &epoll);
#endif