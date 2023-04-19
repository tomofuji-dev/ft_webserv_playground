#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

constexpr int MAX_EVENTS = 10;
constexpr int BUFFER_SIZE = 1024;
constexpr int SERVER_PORT = 8080;

int set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        return -1;
    }
    return 0;
}

int main() {
    int server_fd, epoll_fd;
    struct sockaddr_in server_addr;
    struct epoll_event ev, events[MAX_EVENTS];

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    // Bind server socket to address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Set server socket to non-blocking
    if (set_non_blocking(server_fd) == -1) {
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Create epoll instance
    if ((epoll_fd = epoll_create(1)) == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = server_fd;

    // Add server socket to epoll
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd) {
                // Handle new connection
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }

                if (set_non_blocking(client_fd) == -1) {
                    close(client_fd);
                    continue;
                }

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    perror("epoll_ctl");
                    close(client_fd);
                    continue;
                }

                printf("New client connected: %d\n", client_fd);
            } else {
                // Handle data from client
                int client_fd = events[i].data.fd;
                char buffer[BUFFER_SIZE];
                ssize_t bytes_received;

                while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0) {
                    printf("Received from client %d: %.*s", client_fd, (int)bytes_received, buffer);

                    ssize_t bytes_sent = send(client_fd, buffer, bytes_received, 0);
                    if (bytes_sent == -1) {
                        perror("send");
                        break;
                    }
                }

                if (bytes_received == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("recv");
                }

                printf("Client disconnected: %d\n", client_fd);
                close(client_fd);
            }
        }
    }

    close(server_fd);
    close(epoll_fd);

    return 0;
}
