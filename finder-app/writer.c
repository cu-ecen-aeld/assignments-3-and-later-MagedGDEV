#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <syslog.h>

#include <string.h>

int main (int argc, char *argv[])
{

    openlog("writer.c", LOG_PID, LOG_USER);

    if (argc < 3)
    {
        syslog(LOG_ERR, "Missing arguments");
        closelog();
        return 1;
    }
    
    int fd;
    char* file = argv[1];
    char* string = argv[2];

    fd = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0644);

    if (fd == -1){
        syslog(LOG_ERR, "Can't open, or create file");
        closelog();
        return 1;
    }

    ssize_t nr;
    nr = write(fd, string, strlen(string));
    if (nr == -1){
        syslog(LOG_ERR, "Error when writing %s into %s", string, file);
        closelog();
        return 1;
    }

    syslog(LOG_DEBUG, "Writing %s into %s", string, file);

    if (close(fd) == -1){
        syslog(LOG_ERR, "Error when closing %s", file);
        closelog();
        return 1;
    }

    closelog();

    return 0;
}