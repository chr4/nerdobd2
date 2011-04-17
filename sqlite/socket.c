#include "sqlite.h"
#include "../common/config.h"

void cleanup(int);
void cut_crlf(char *);
int  calculate_consumption(void);


// child pids
pid_t  handler;
pid_t  syncer;

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

int
calculate_consumption(void)
{
    char  query[1024];
    float per_h;
    float per_km;
    float speed;

    // calculate consumption per hour
    per_h = 60 * 4 * MULTIPLIER * get_value("rpm") * (get_value("injection_time") - INJ_SUBTRACT);

    // calculate consumption per hour
    if ( (speed = get_value("speed")) > 0)
        per_km = per_h / speed * 100;
    else
        per_km = -1;

    snprintf(query, sizeof(query),
        "INSERT INTO consumption VALUES ( \
             NULL, DATETIME('NOW'), %f, %f )",
        per_h, per_km);

    return exec_query(query);
}



// TODO: this function needs to be nicer massively
int
handle_client(int c)
{
    int  n;
    char buf[1024];
    char *key, *value;

    // remove signal handlers
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);

    n = read(c, buf, 1024);
    buf[n] = '\0';

    cut_crlf(buf);

    // parse key
    if ( (key = strtok(buf, ":")) == NULL)
    {
        printf("error: no key found.\n");
        return -1;
    }

    // check whether key tells us to calculate consumption
    if (!strcmp(buf, "calculate_consumption"))
    {
        calculate_consumption();
        close(c);
        return 0;
    }

    // parse value
    if ( (value = strtok(NULL, ":")) == NULL)
    {
        printf("error: no value found.\n");
        return -1;
    }

    // insert key value pair to database
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


    // add signal handler for cleanup function
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    // wait for children when dead
    signal(SIGCHLD, sig_chld);


    // initialize db
    if (init_db() == -1)
        return -1;

    /* 
    // sync the database from ram do disk every few seconds
    if ( (syncer = fork()) == 0)
    {
        // remove signal handlers
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);

        for ( ; ; )
        {
            sync_db();
            sleep(10);
        }
    }
    */

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

