#ifndef AESDSOCKET_H
#define AESDSOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>

// Helper functions
int create_dir_delete_old_content(int sockfd);
void print_data(const char* data, int* data_size);
int daemonize(void);

// Server
int init_socket(struct addrinfo* hints, struct addrinfo** servinfo);
int start_listening(int sockfd, struct addrinfo** servinfo);
int send_current_data(int clientfd);

// Client
int connect_client(int sockfd, struct sockaddr* addr, socklen_t* addr_size);
char* receive_data(int clientfd, int* data_size);

#endif