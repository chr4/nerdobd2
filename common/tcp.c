#include <stdio.h>
#include <unistd.h>
#include <error.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

void
sig_chld(int signo)
{
        pid_t pid;
        int   stat;

        while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0);
        return;
}


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
    struct sockaddr_in cliaddr;

    signal(SIGCHLD, sig_chld);

    // accept incoming connections
    for ( ; ; )
    {
        clisize = sizeof (cliaddr);
        if ((c = accept (s, (struct sockaddr *) &cliaddr,
                         (socklen_t *) & clisize)) == -1)
            continue;


        if (fork () == 0)
        {
            // remove signal handlers
            signal(SIGINT, SIG_DFL);
            signal(SIGTERM, SIG_DFL);

            close(s);
            callback(c);
            close (c);

            _exit(0);
        }

        close (c);
    }
}
