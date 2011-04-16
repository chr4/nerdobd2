#include "sqlite.h"
#include "../common/config.h"

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

    printf("%s -> %s\n", key, value);
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
    pid_t  child;


    // initialize db
    if (init_db() == -1)
        return -1;

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
        if( (child = fork()) == 0)
            return handle_client(c);
        close(c);
    }

    close(s);
    unlink(UNIX_SOCKET);

    return 0;
}

