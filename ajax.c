#include "serial.h"

int     tcp_listen (int);
int     handle_client (int);
void    cut_crlf (char *);
ssize_t readline (int, void *, size_t);
char   *get_line (FILE *);
FILE   *open_html (char *);


int
ajax_socket (int port)
{
    int     cli, srv;
    int     clisize;
	
    struct sockaddr_in cliaddr;
	
	
    srv = tcp_listen (port);
	
    for (;;)
    {
		pthread_t th_client;
		
		clisize = sizeof (cliaddr);
		if ((cli = accept (srv, &cliaddr, &clisize)) == -1)
			continue;
		
		pthread_create( &th_client, NULL, handle_client, cli);

		close (cli);
    }
	
    return 0;
}

int
tcp_listen (int port)
{
	
    int     on = 1;
    int     sock;
	
    struct sockaddr_in servaddr;
	
    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
		perror ("socket() failed");
		exit (-1);
    }
	
    setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));
	
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons (port);
	
    if (bind (sock, &servaddr, sizeof (servaddr)) == -1)
    {
		perror ("bind() failed");
		exit (-1);
    }
	
    if (listen (sock, 3) == -1)
    {
		perror ("listen() failed");
		exit (-1);
    }
	
    return sock;
}

int
handle_client (int connfd)
{
    char    recv_buf[1024];
    char    send_buf[1024];
	
    FILE   *fd;
    char   *line;
	
	
    if (readline (connfd, recv_buf, sizeof (recv_buf)) <= 0)
		return 0;
	
    cut_crlf (recv_buf);
	
    if (!strcmp (recv_buf, "GET / HTTP/1.1"))
    {
		printf ("GET ");
		// read and ignore all http headers
		while (strcmp (recv_buf, ""))
		{
			if (readline (connfd, recv_buf, sizeof (recv_buf)) <= 0)
				return 0;
			
			cut_crlf (recv_buf);
		}
		
		// send the html file
		fd = open_html ("ajax.html");
		
		printf ("ajax.html\n");
		while ((line = get_line (fd)) != NULL)
		{
			send (connfd, line, strlen (line), 0);
		}
    }
	
    else if (!strcmp (recv_buf, "POST /update_consumption HTTP/1.1"))
    {
		//printf("POST ");
		
		// read and ignore headers
		while (strcmp (recv_buf, ""))
		{
			if (readline (connfd, recv_buf, sizeof (recv_buf)) <= 0)
				return 0;
			
			cut_crlf (recv_buf);
		}
		
		char    buf[256];
		snprintf (buf, sizeof (buf), "%.02f", l_per_100km);
		//printf("%s\n", buf);
		send (connfd, buf, strlen (buf), 0);
    }
    else if (!strcmp (recv_buf, "POST /update_speed HTTP/1.1"))
    {
		// printf("POST ");
		
		// read and ignore headers
		while (strcmp (recv_buf, ""))
		{
			if (readline (connfd, recv_buf, sizeof (recv_buf)) <= 0)
				return 0;
			
			cut_crlf (recv_buf);
		}
		
		char    buf[256];
		snprintf (buf, sizeof (buf), "%.01f", speed);
		//printf("%s\n", buf);
		send (connfd, buf, strlen (buf), 0);
    }
    else
    {
		printf ("got something else (%s)\n", recv_buf);
    }
	
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



FILE   *
open_html (char *file)
{
	
    FILE   *fd;
	
    if ((fd = fopen (file, "r")) == NULL)
    {
		perror ("could not read html file");
		exit (-1);
    }
	
    return fd;
}
