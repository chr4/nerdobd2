/*
 * this process does nothing but creating a sqlite database,
 * listening on a tcp socket and forwarding queries to it
 * this is a different process and necessary, because we cannot
 * compile core with -lsqlite, this will break the serial connection
 * due to mysterious reasons
 */

#include "sqlite.h"
#include "../common/tcp.h"


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
void
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
}

int
main (int argc, char **argv)
{
    int s;

    // add signal handler for cleanup function
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    // initialize db
    if (init_db() == -1)
        return -1;

    if ( (s = tcp_listen(DB_PORT)) == -1)
        return -1;

    tcp_loop_accept(s, &handle_client);

    // should never be reached
    close(s);
    return 0;
}
