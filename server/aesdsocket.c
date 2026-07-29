#include "aesdsocket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <syslog.h>

#include "../examples/systemcalls/systemcalls.h"

int CLIENT_COUNT = 5;
mode_t DIR_MODE = 0777;

const char* PORT = "9000";
const char* DIR_NAME = "/var/tmp";
const char* FILE_NAME = "/var/tmp/aesdsocketdata";


int create_dir_path(const char* path, mode_t mode)
{
    char tmp[256];
    char *p = NULL;
    size_t len;

    strcpy(tmp, path);
    len = strlen(tmp);
    if (len == 0)
        return -1;

    if (tmp[len - 1] == '/')
        tmp[len - 1] = '\0';

    for (p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = '\0';

            if (mkdir(tmp, mode) != 0 && errno != EEXIST)
                return -1;
            
            *p = '/';
        }
    }

    if (mkdir(tmp, mode) != 0 && errno != EEXIST)
        return -1;

    return 0;
}

int create_dir_delete_old_content(int sockfd)
{
    int status;
    if((status = create_dir_path(DIR_NAME, DIR_MODE)) == -1)
    {
        printf("Failed to create path\n");
        close(sockfd);
        closelog();
        return -1;
    }

    if (unlink(FILE_NAME) == 0)
        printf("File deleted successfully\n");
    else 
    {
        if (errno ==  ENOENT)
            printf("File doesn't exist\n");
        else
        {
            printf("Error deleting file\n");
            close(sockfd);
            closelog();
            return -1;
        } 
    }

    return 0;
}

void print_data(const char* data, int* data_size)
{
    char *home = getenv("HOME");
    if (home == NULL)
    {
        printf("HOME is not set\n");
        exit(-1);
    }

    char* cmd = malloc(*data_size + 100);
    snprintf(cmd,
        (*data_size + 100) * sizeof(char), 
        "%s/bin/writer /var/tmp/aesdsocketdata \"%s\"",
        home, 
        data
    );
    do_system(cmd);
    free(cmd);
}

int daemonize(void)
{
    pid_t pid;

    pid = fork();
    if (pid < 0)
        return -1;

    if (pid > 0)
        exit(EXIT_SUCCESS);    /* Parent exits */

    if (setsid() == -1)
        return -1;

    /* Second fork */
    pid = fork();
    if (pid < 0)
        return -1;

    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* Clear file mode creation mask */
    umask(0);

    /* Change working directory */
    if (chdir("/") == -1)
        return -1;

    /* Close standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Redirect them to /dev/null */
    int fd = open("/dev/null", O_RDWR);
    if (fd >= 0)
    {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);

        if (fd > STDERR_FILENO)
            close(fd);
    }

    return 0;
}

int init_socket(struct addrinfo* hints, struct addrinfo** servinfo)
{
    memset(hints, 0, sizeof (*hints));
    hints->ai_flags = AI_PASSIVE;
    hints->ai_family = AF_INET;
    hints->ai_socktype = SOCK_STREAM;

    if (getaddrinfo(NULL, PORT, hints, servinfo) != 0)
    {
        printf("Failed to fill addrinfo\n");
        closelog();
        return -1;
    }

    int sockfd;
    if ((sockfd = socket((*servinfo)->ai_family, (*servinfo)->ai_socktype, (*servinfo)->ai_protocol)) == -1)
    {
        printf("Failed to create socket\n");
        closelog();
        return -1;
    }

    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        perror("setsockopt");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int start_listening(int sockfd, struct addrinfo** servinfo)
{
    if (bind(sockfd, (*servinfo)->ai_addr, (*servinfo)->ai_addrlen) == -1)
    {
        printf("Failed to bind socket\n");
        close(sockfd);
        closelog();
        return -1;
    }

    if (listen(sockfd, CLIENT_COUNT) == -1)
    {
        printf("Failed to listen to socket\n");
        close(sockfd);
        closelog();
        return -1;
    }

    return 0;
}

int send_current_data(int clientfd)
{
    int fd = open("/var/tmp/aesdsocketdata", O_RDONLY);

    if (fd == -1)
    {
        printf("Failed to read file to send current data\n");
        close(clientfd);
        return -1;
    }

    char buffer[1024];
    ssize_t bytes_read;

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        int bytes_sent;
        if((bytes_sent = send(clientfd, buffer, bytes_read, 0)) <= 0)
        {
            printf("Failed to send data to client\n");
            close(clientfd);
            return -1;
        }
    }

    if (bytes_read == -1)
        printf("Failed to read data\n");
    close(fd);
    
    return 0;
}

int connect_client(int sockfd, struct sockaddr* addr, socklen_t* addr_size)
{
    int clientfd;
    if ((clientfd = accept(sockfd, addr, addr_size)) == -1)
    {
        printf("Failed to accept the socket\n");
        close(sockfd);
        closelog();
        return -1;
    }
    return clientfd;
}

char* receive_data(int clientfd, int* data_size)
{
    size_t capacity = 1024;
    size_t length = 0;

    char *buffer = malloc(capacity);
    if (buffer == NULL)
        return NULL;

    while (1)
    {
        /* Grow the buffer if necessary */
        if (length == capacity - 1)
        {
            capacity *= 2;

            char *tmp = realloc(buffer, capacity);
            if (tmp == NULL)
            {
                free(buffer);
                return NULL;
            }

            buffer = tmp;
        }

        ssize_t bytes = recv(clientfd,
            buffer + length,
            capacity - length - 1,
            0
        );

        if (bytes <= 0)
        {
            free(buffer);
            return NULL;
        }

        length += bytes;
        *data_size = length;
        buffer[length] = '\0';

        if (memchr(buffer, '\n', length) != NULL)
            break;
    }

    return buffer;
}