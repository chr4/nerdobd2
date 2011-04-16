#include "sqlite.h"
#include "../common.h"

int
handle_client(int c)
{
    int  n;
    char buf[1024];

    n = read(c, buf, 1024);
    buf[n] = '\0';

    printf("message from obd: %s\n", buf);

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

