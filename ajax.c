#include "serial.h"

#define HEADER_PLAIN    "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\n\r\n"
#define HEADER_PNG      "HTTP/1.0 200 OK\r\nContent-Type: image/png\r\n\r\n"

int     tcp_listen (int);
int     handle_client (int);
void    cut_crlf (char *);
ssize_t readline (int, void *, size_t);
char   *get_line (FILE *);
int     ignore_headers (int);


void   *
ajax_socket (void *pport)
{
    int     port;
    int     status;
    int     clisize;
    pid_t   pid;
    port = (int) pport;
    int     cli;
    struct sockaddr_in cliaddr;
    
    tcp_listen (port);

    for (;;)
    {
        // pthread_t th_client;

        clisize = sizeof (cliaddr);
        if ((cli = accept (srv, (struct sockaddr *) &cliaddr,
                           (socklen_t *) & clisize)) == -1)
            continue;

        // pthread throws strange errors after a while, thus using fork()
        // pthread_create( &th_client, NULL, handle_client, (void *) cli);
        
        if ((pid = fork ()) == 0)
        {
            close (srv);
            handle_client (cli);
            close (cli);
            exit (0);
        }

        waitpid (pid, &status, 0);

        close (cli);
    }
}

int
ajax_shutdown(void)
{
    /*
     * we need to shutdown the socket to prevent
     * bind errors on direct relaunch
     */
     
    printf("shutting down socket... ");
    if ( shutdown(srv, SHUT_RDWR) == -1)
    {
        perror("shutdown() failed:");
        return -1;
    }
    printf("done.\n");
    
    return 0;
}

int
tcp_listen (int port)
{

    int     on = 1;

    struct sockaddr_in servaddr;

    if ((srv = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror ("socket() failed");
        pthread_exit (NULL);
    }

    setsockopt (srv, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));

    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons (port);

    while (bind (srv, (struct sockaddr *) &servaddr, sizeof (servaddr)) == -1)
    {
        perror ("bind() failed");
        usleep(50000);
        printf("retrying...\n");        
    }

    if (listen (srv, 3) == -1)
    {
        perror ("listen() failed");
        pthread_exit (NULL);
    }

    return 0;
}

int
handle_client (int connfd)
{
    char    recv_buf[1024];
    FILE   *fd;
    char   *line;

    if (readline (connfd, recv_buf, sizeof (recv_buf)) <= 0)
        return -1;

    cut_crlf (recv_buf);

    if (!strcmp (recv_buf, "GET / HTTP/1.1"))
    {
#ifdef DEBUG
        printf ("GET ajax.html\n");
#endif
        // read and ignore all http headers
        if (ignore_headers (connfd) == -1)
            return 0;

        // send the html file
        if ((fd = fopen ("ajax.html", "r")) == NULL)
        {
            perror ("could not read html file");
            exit (-1);
        }

        while ((line = get_line (fd)) != NULL)
        {
            send (connfd, line, strlen (line), 0);
        }
        fclose (fd);
    }

    else if (strstr (recv_buf, "GET /speed.png"))
    {
        int     file_fd;
        int     ret;
        static char buffer[1024 + 1];        /* static so zero filled */

#ifdef DEBUG
        printf ("GET speed.png\n");
#endif
        // read and ignore all http headers
        if (ignore_headers (connfd) == -1)
            return 0;

        if ((file_fd = open("speed.png", O_RDONLY)) == -1)
        {
            perror ("couldnt open png file\n");
            return -1;
        }
        write (connfd, HEADER_PNG, strlen(HEADER_PNG));

        while ((ret = read (file_fd, buffer, 1024)) > 0)
        {

            (void) write (connfd, buffer, ret);

        }
        close (file_fd);
    }
    else if (strstr (recv_buf, "GET /consumption.png"))
    {
        int     file_fd;
        int     ret;
        static char buffer[1024 + 1];        /* static so zero filled */

#ifdef DEBUG
        printf ("GET consumption.png\n");
#endif
        // read and ignore all http headers
        if (ignore_headers (connfd) == -1)
            return 0;

        if ((file_fd = open ("consumption.png", O_RDONLY)) == -1)
        {
            perror ("couldnt open png file\n");
            return -1;
        }

        write (connfd, HEADER_PNG, strlen (HEADER_PNG));

        while ((ret = read (file_fd, buffer, 1024)) > 0)
        {

            (void) write (connfd, buffer, ret);

        }
        close (file_fd);
    }

    else if (!strcmp (recv_buf, "POST /update_con_km HTTP/1.1"))
    {        
        // read and ignore headers
        if (ignore_headers (connfd) == -1)
            return 0;

        char    buf[256];
        snprintf (buf, sizeof (buf), "%.02f", con_km);
        send (connfd, HEADER_PLAIN, strlen (HEADER_PLAIN), 0);
        send (connfd, buf, strlen (buf), 0);
    }
    else if (!strcmp (recv_buf, "POST /update_speed HTTP/1.1"))
    {
        // read and ignore headers
        if (ignore_headers (connfd) == -1)
            return 0;

        char    buf[256];
        snprintf (buf, sizeof (buf), "%.01f", speed);
        send (connfd, HEADER_PLAIN, strlen (HEADER_PLAIN), 0);
        send (connfd, buf, strlen (buf), 0);
    }
    else if (!strcmp (recv_buf, "POST /update_rpm HTTP/1.1"))
    {
        // read and ignore headers
        if (ignore_headers (connfd) == -1)
            return 0;

        char    buf[256];
        snprintf (buf, sizeof (buf), "%.00f", rpm);
        send (connfd, HEADER_PLAIN, strlen (HEADER_PLAIN), 0);
        send (connfd, buf, strlen (buf), 0);
    }
    else if (!strcmp (recv_buf, "POST /update_con_h HTTP/1.1"))
    {
        // read and ignore headers
        if (ignore_headers (connfd) == -1)
            return 0;

        char    buf[256];
        snprintf (buf, sizeof (buf), "%.02f", con_h);
        send (connfd, HEADER_PLAIN, strlen (HEADER_PLAIN), 0);
        send (connfd, buf, strlen (buf), 0);
    }
    else if (!strcmp (recv_buf, "POST /update_load HTTP/1.1"))
    {
        // read and ignore headers
        if (ignore_headers (connfd) == -1)
            return 0;

        char    buf[256];
        snprintf (buf, sizeof (buf), "%.00f", load);
        send (connfd, HEADER_PLAIN, strlen (HEADER_PLAIN), 0);
        send (connfd, buf, strlen (buf), 0);
    }
    else if (!strcmp (recv_buf, "POST /update_temp1 HTTP/1.1"))
    {
        // read and ignore headers
        if (ignore_headers (connfd) == -1)
            return 0;

        char    buf[256];
        snprintf (buf, sizeof (buf), "%.01f", temp1);
        send (connfd, HEADER_PLAIN, strlen (HEADER_PLAIN), 0);
        send (connfd, buf, strlen (buf), 0);
    }
    else if (!strcmp (recv_buf, "POST /update_temp2 HTTP/1.1"))
    {
        // read and ignore headers
        if (ignore_headers (connfd) == -1)
            return 0;

        char    buf[256];
        snprintf (buf, sizeof (buf), "%.01f", temp2);
        send (connfd, HEADER_PLAIN, strlen (HEADER_PLAIN), 0);
        send (connfd, buf, strlen (buf), 0);
    }
    else if (!strcmp (recv_buf, "POST /update_voltage HTTP/1.1"))
    {
        // read and ignore headers
        if (ignore_headers (connfd) == -1)
            return 0;

        char    buf[256];
        snprintf (buf, sizeof (buf), "%.02f", voltage);
        send (connfd, HEADER_PLAIN, strlen (HEADER_PLAIN), 0);
        send (connfd, buf, strlen (buf), 0);
    }
#ifdef DEBUG    
    else
        printf ("got something else (%s)\n", recv_buf);
#endif

    return 0;
}

// read and ignore headers
int
ignore_headers (int fd)
{
    char    recv_buf[1024];

    do
    {
        if (readline (fd, recv_buf, sizeof (recv_buf)) <= 0)
            return -1;

        cut_crlf (recv_buf);

    } while (strcmp (recv_buf, ""));

    return 0;
}

void
cut_crlf (char *stuff)
{

    char   *p;

    p = strchr (stuff, '\r');
    if (p)
        *p = '\0';

    p = strchr (stuff, '\n');
    if (p)
        *p = '\0';
}

ssize_t
readline (int fd, void *vptr, size_t maxlen)
{

    ssize_t n, rc;
    char    c, *ptr;

    ptr = vptr;

    for (n = 1; n < maxlen; n++)
    {

      again:
        if ((rc = read (fd, &c, 1)) == 1)
        {
            *ptr++ = c;
            if (c == '\n')
                break;
        }
        else if (rc == 0)
        {
            if (n == 1)
                return 0;
            else
                break;
        }
        else
        {
            if (errno == EINTR)
                goto again;
            return -1;
        }
    }

    *ptr = 0;
    return n;
}


char   *
get_line (FILE * fz)
{
    char    buffy[1024];
    char   *p;

    if (fgets (buffy, 1024, fz) == NULL)
        return NULL;

    // cut_crlf(buffy);
    p = buffy;

    return p;
}