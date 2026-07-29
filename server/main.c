#include "aesdsocket.h"

#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

bool caught_sig = false;
struct addrinfo *servinfo;
const char* PROGRAM_NAME = "aesdsocket";

static void signal_handler(int signal_number);
int register_signals(struct sigaction* new_action);

int main (int argc, char *argv[])
{
    openlog(PROGRAM_NAME, LOG_PID, LOG_USER);

    int status;
    struct addrinfo hints;
    
    struct sockaddr addr;
    socklen_t addr_size = sizeof(addr);

    int sockfd;
    if ((sockfd = init_socket(&hints, &servinfo)) == -1)
        return -1;

    if (start_listening(sockfd, &servinfo) == -1)
        return -1;

    if (create_dir_delete_old_content(sockfd) == -1)
        return -1;

    if (argc == 2)
    {
        if (strcmp(argv[1], "-d") == 0)
        {
            printf("Daemon mode enabled\n");
            
            if (daemonize() == -1)
            {
                printf("Daemon mode failed\n");
                return EXIT_FAILURE;
            }
        }
    }

    struct sigaction new_action;
    if (register_signals(&new_action) == -1)
        return -1;

    while(!caught_sig)
    {
        int clientfd;
        if((clientfd = connect_client(sockfd, &addr, &addr_size)) == -1)
            return -1;

        syslog(LOG_DEBUG, "Accepted connection from %s", addr.sa_data);

        int data_size;
        char* data_received = receive_data(clientfd, &data_size);
        print_data(data_received, &data_size);
        free(data_received);

        send_current_data(clientfd);
        
        close(clientfd);
        syslog(LOG_DEBUG, "Closed connection from %s", addr.sa_data);
    }

    close(sockfd);
    freeaddrinfo(servinfo);
    closelog();
    
    return 0;
}

static void signal_handler(int signal_number)
{
    if (signal_number == SIGINT || signal_number == SIGTERM)
    {
        caught_sig = true;
        syslog(LOG_DEBUG, "Caught signal, exiting");
        freeaddrinfo(servinfo);
        closelog();
    } 
}

int register_signals(struct sigaction* new_action)
{
    memset(new_action, 0, sizeof (*new_action));
    new_action->sa_handler = signal_handler;
    if (sigaction(SIGTERM, new_action, NULL) != 0)
    {
        printf("Failed to register SIGTERM\n");
        return -1;
    }
    if (sigaction(SIGINT, new_action, NULL) != 0)
    {
        printf("Failed to register SIGINT\n");
        return -1;
    }
    return 0;
}