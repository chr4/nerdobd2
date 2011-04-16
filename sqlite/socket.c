#include "sqlite.h"
#include "../common/config.h"

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

    printf("syncing db file to harddisk...\n");
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
int
handle_client(int c)
{
    int  n;
    char buf[1024];
    char *key, *value;

    n = read(c, buf, 1024);
    buf[n] = '\0';

    cut_crlf(buf);

    // split string
    if ( (key = strtok(buf, ":")) == NULL)
    {
        printf("error: no key found.\n");
        return -1;
    }

    if ( (value = strtok(NULL, ":")) == NULL)
    {
        printf("error: no value found.\n");
        return -1;
    }

    insert_value(key, atof(value));

    close(c);
    return 0;
}

int
main (int argc, char **argv)
{
    // unix domain sockets
    struct sockaddr_un address;
    size_t address_length;
    int    s, c;

    // child pids
    pid_t  handler;
    pid_t  syncer;


    // initialize db
    if (init_db() == -1)
        return -1;

    
    // sync the database from ram do disk every few seconds
    if ( (syncer = fork()) == 0)
    {
        // add signal handler for cleanup function
        signal(SIGINT, cleanup);
        signal(SIGTERM, cleanup);

        for ( ; ; )
        {
            sync_db();
            sleep(3);
        }
    }

    // create unix socket
    if ( (s = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() failed");
        return -1;
    }

    unlink(UNIX_SOCKET);
    address.sun_family = AF_UNIX;
    address_length = sizeof(address.sun_family) +
                     sprintf(address.sun_path, UNIX_SOCKET);

    if (bind(s, (struct sockaddr *) &address, address_length) != 0)
    {
        perror("bind() failed");
        return -1;
    }

    if (listen(s, 5) != 0)
    {
        perror("listen() failed");
        return -1;
    }

    // accept incoming connections
    while ((c = accept(s, (struct sockaddr *) &address, &address_length)) > 1)
    {
        if( (handler = fork()) == 0)
            return handle_client(c);
        close(c);
    }

    close(s);
    unlink(UNIX_SOCKET);

    return 0;
}

