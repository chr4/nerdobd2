#include <stdio.h>
#include <unistd.h>
#include <error.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

int
tcp_listen(int port)
{
    int s;
    int on = 1;
    struct sockaddr_in servaddr;
    
    if ((s = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror ("socket() failed");
        return -1;
    }
    
    setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));
    
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons (port);
    
    // retry if bind failed
    while (bind (s, (struct sockaddr *) &servaddr, sizeof (servaddr)) == -1)
    {
        perror ("bind() failed");
        usleep(50000);
        printf("retrying...\n");
    }
    
    if (listen (s, 3) == -1)
    {
        perror ("listen() failed");
        return -1;
    }

    return s;
}

void
tcp_loop_accept(int s, void (*callback)(int))
{
    int    c, clisize;
    int    status;
    struct sockaddr_in cliaddr;

    // accept incoming connections
    for ( ; ; )
    {
        clisize = sizeof (cliaddr);
        if ((c = accept (s, (struct sockaddr *) &cliaddr,
                         (socklen_t *) & clisize)) == -1)
            continue;


        if (fork () == 0)
        {
            close(s);

            callback(c);

            close (c);
            _exit(0);
        }

        // collect defunct processes (don't wait)
        while(waitpid(-1, &status, WNOHANG) > 0);

        close (c);
    }
}
