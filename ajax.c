#include "serial.h"

#define HEADER_PLAIN    "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\n\r\n"
#define HEADER_HTML     "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
#define HEADER_PNG      "HTTP/1.0 200 OK\r\nContent-Type: image/png\r\n\r\n"
#define HEADER_CSS      "HTTP/1.0 200 OK\r\nContent-Type: text/css\r\n\r\n"
#define HEADER_JS       "HTTP/1.0 200 OK\r\nContent-Type: application/x-javascript\r\n\r\n"
#define HEADER_ICON     "HTTP/1.0 200 OK\r\nContent-Type: image/x-icon\r\n\r\n"

/*
 extensions [] = {
 
 {"gif", "image/gif" },  
 
 {"jpg", "image/jpeg"}, 
 
 {"jpeg","image/jpeg"},
 
 {"png", "image/png" },  
 
 {"zip", "image/zip" },  
 
 {"gz",  "image/gz"  },  
 
 {"tar", "image/tar" },  
 
 {"htm", "text/html" },  
 
 {"html","text/html" },  
 
 {0,0} };

 
 buflen=strlen(buffer);
 
 fstr = (char *)0;
 
 for(i=0;extensions[i].ext != 0;i++) {
 
 len = strlen(extensions[i].ext);
 
 if( !strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
 
 fstr =extensions[i].filetype;
 
 break;
 
 }
 
 }
 */


int     tcp_listen (int);
int     handle_client(int);
int     obd_send(int, float, char *);


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

    for ( ; ; )
    {
        clisize = sizeof (cliaddr);
        if ((cli = accept (srv, (struct sockaddr *) &cliaddr,
                           (socklen_t *) & clisize)) == -1)
            continue;

        
        if ((pid = fork ()) == 0)
        {
            close (srv);
            handle_client (cli);
            close (cli);
            exit (0);
        }

        // collect defunct processes (don't wait)
        while(waitpid(-1, &status, WNOHANG) > 0);
        
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
obd_send(int fd, float val, char *format)
{
    char buf[256];
    
    // check if value was set
    if (val == -2)
        return -1;
    
    snprintf (buf, sizeof (buf), format, val);
    send (fd, HEADER_PLAIN, strlen (HEADER_PLAIN), 0);
    send (fd, buf, strlen (buf), 0);
    
    return 0;
}


int
handle_client(int fd)
{
    
    int file_fd;
    int r, i, j;
    static char buffer[1024];
    char *p;
    
    // read socket
    if ( (r = read (fd, buffer, sizeof(buffer))) < 0)
    {
        printf("read() failed\n");
        return -1;
    }
    
    // terminate received string
    if ( r > 0 && r < sizeof(buffer))
        buffer[r] = '\0';
    else
        buffer[0] = '\0';
    
    // cut \n \r
    for (i = 0; i < r; i++)
        if(buffer[i] == '\r' || buffer[i] == '\n')
            buffer[i]='#';

    // printf("request: %s\n", buffer);
    
    
    if (strncmp(buffer,"GET ", 4) && strncmp(buffer,"POST ", 5) )
    {
        printf("something else than GET and POST received\n");
        return -1;
    }
    
    // look for second space and terminate string (skip headers)
    if ( (p = strchr(buffer, ' ')) != NULL)
        if ( (p = strchr(p+1, ' ')) != NULL)
            *p = '\0';
    
    
    // check for illegal parent directory requests
    for (j = 0; j < i - 1; j++)
    {
        if(buffer[j] == '.' && buffer[j+1] == '.')
        {
            printf(".. detected\n");
            return -1;
        }
    }
    
    // convert / to ajax.html
    if (!strncmp(buffer, "GET /\0", 6)) 
        strncpy(buffer, "GET /ajax.html", sizeof(buffer));
    
    
    // answer to our obd requests ( get.obd?varname )
    else if (!strncmp(buffer, "POST /get.obd?", 1) )
    {      
        // which varname?
        if ( (p = strchr(buffer, '?')) == NULL)
        {
            printf("bad obd request\n");
            return -1;
        }
        p++;    // skip '?'
        
#ifdef DEBUG
        printf("got obd request for %s\n", p);
#endif
        
        if (!strcmp(p, "speed") )
            obd_send(fd, speed, "%.01f");
        else if (!strcmp(p, "rpm") )
            obd_send(fd, rpm, "%.00f");
        else if (!strcmp(p, "con_h") )
            obd_send(fd, con_h, "%.02f");
        else if (!strcmp(p, "con_km") )
            obd_send(fd, con_km, "%.02f");
        else if (!strcmp(p, "load") )
            obd_send(fd, load, "%.00f");
        else if (!strcmp(p, "temp1") )
            obd_send(fd, temp1, "%.01f");
        else if (!strcmp(p, "temp2") )
            obd_send(fd, temp2, "%.00f");
        else if (!strcmp(p, "voltage") )
            obd_send(fd, voltage, "%.02f");
        else
        {
            printf("unkown obd varname: %s\n", p);
            return -1;
        }
        
        return 0;
    }
    
    // terminate arguments after ?
    if ( (p = strchr(buffer, '?')) != NULL) 
        *p = '\0';
   
#ifdef DEBUG
    printf("%s\n", buffer);
#endif
    
    // search for last dot in filename
    if ( (p = strrchr(buffer, '.')) == NULL)
    {
        printf("no . found in filename!\n");
        return -1;
    }
        
    // is file type known?
    if ( !strcmp(p, ".html") ||  !strcmp(p, ".htm") )
        write(fd, HEADER_HTML, strlen(HEADER_HTML));
    
    else if (!strcmp(p, ".png") )
        write(fd, HEADER_PNG, strlen(HEADER_PNG));
    
    else if (!strcmp(p, ".txt") )
        write(fd, HEADER_PLAIN, strlen(HEADER_PLAIN));   
    
    else if (!strcmp(p, ".js") )
        write(fd, HEADER_JS, strlen(HEADER_JS));
    
    else if (!strcmp(p, ".css") )
        write(fd, HEADER_CSS, strlen(HEADER_CSS));
    
    else if (!strcmp(p, ".ico") )
        write(fd, HEADER_ICON, strlen(HEADER_ICON));    
    
    else
    {
        printf("extention not found\n");
        return -1;
    }
    
    if(( file_fd = open(&buffer[5],O_RDONLY)) == -1)
    {
        printf("open() failed: %s\n", &buffer[5]);
        return -1;
    }
    
    while ( (r = read(file_fd, buffer, sizeof(buffer))) > 0 )
        write(fd, buffer,r);

    close(file_fd);
    
    return 0;
}