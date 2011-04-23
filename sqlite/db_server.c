#include "sqlite.h"

// child pids
pid_t  handler;


// flag for cleanup function
char    cleaning_up = 0;

void
cleanup (int signo)
{
    // if we're already cleaning up, do nothing
    if (cleaning_up)
        return;

    cleaning_up = 1;

    printf("\ncleaning up:\n");
    sync_db();

    printf("exiting.\n");
    exit(0);
}

void
sig_chld(int signo)
{
        pid_t   pid;
        int     stat;

        while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0);
        return;
}


// cut newlines
void
cut_crlf(char *s) {

        char *p;

        p = strchr(s, '\r');
        if (p)
                *p = '\0';

        p = strchr(s, '\n');
        if (p)
                *p = '\0';
}


// TODO: this function needs to be nicer massively
int
handle_client(int c)
{
    int  n;
    char buf[LEN_QUERY];

    // remove signal handlers
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);

    n = read(c, buf, sizeof(buf));
    buf[n] = '\0';

    cut_crlf(buf);

    exec_query(buf);

    close(c);
    return 0;
}

int
setup_socket(void)
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
    servaddr.sin_port = htons (DB_PORT);
    
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

int
main (int argc, char **argv)
{
    struct sockaddr_in cliaddr;
    int    s, c;
    int    clisize;



    // add signal handler for cleanup function
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    // wait for children when dead
    signal(SIGCHLD, sig_chld);


    // initialize db
    if (init_db() == -1)
        return -1;

    if ( (s = setup_socket()) == -1)
        return -1;

    // accept incoming connections
    for ( ; ; )
    {
        clisize = sizeof (cliaddr);
        if ((c = accept (s, (struct sockaddr *) &cliaddr,
                         (socklen_t *) & clisize)) == -1)
            continue;


        if ((handler = fork ()) == 0)
        {
            close(s);

            handle_client(c);

            close (c);
            _exit(0);
        }

        close (c);
    }

    // should never be reached
    close(s);
    return 0;
}
