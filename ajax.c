#include "serial.h"

#define SERVER_STRING   "Server: nerdobd ajax server |0.9.4\r\n"
#define SERVER_CON      "Connection: close\r\n"
#define HTTP_OK         "HTTP/1.0 200 OK\r\n"

#define HEADER_PLAIN    SERVER_STRING SERVER_CON "Content-Type: text/plain\r\n\r\n"
#define HEADER_HTML     SERVER_STRING SERVER_CON "Content-Type: text/html\r\n\r\n"
#define HEADER_PNG      SERVER_STRING SERVER_CON "Content-Type: image/png\r\n\r\n"
#define HEADER_CSS      SERVER_STRING SERVER_CON "Content-Type: text/css\r\n\r\n"
#define HEADER_JS       SERVER_STRING SERVER_CON "Content-Type: application/x-javascript\r\n\r\n"
#define HEADER_ICON     SERVER_STRING SERVER_CON "Content-Type: image/x-icon\r\n\r\n"


int     tcp_listen (int);
int     handle_client(int);
int     obd_send(int, float, char *);
int     obd_send_debug(int);
void    reset_counters(void);


void
ajax_socket (int port)
{
    int     status;
    int     clisize;
    pid_t   pid;
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
            exit(-1);
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
     
    printf("shutting down socket...\n");
    if ( shutdown(srv, SHUT_RDWR) == -1)
    {
        perror("shutdown() failed:");
        exit(-1);
        return -1;
    }
    printf("done.\n");
    
    exit(0);
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
        exit(-1);
    }

    setsockopt (srv, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));

    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons (port);

    // retry if bind failed
    while (bind (srv, (struct sockaddr *) &servaddr, sizeof (servaddr)) == -1)
    {
        perror ("bind() failed");
        usleep(50000);
        printf("retrying...\n");        
    }

    if (listen (srv, 3) == -1)
    {
        perror ("listen() failed");
        exit(-1);
    }

    return 0;
}



int
obd_send_debug(int fd)
{
    char buf2[256];

    snprintf (buf2, sizeof (buf2), "Content-Length: %d\r\n", strlen(debug));
    send (fd, HTTP_OK, strlen(HTTP_OK), 0);
    send (fd, buf2, strlen(buf2), 0);
    send (fd, HEADER_PLAIN, strlen(HEADER_PLAIN), 0);
    send (fd, debug, strlen (debug), 0);

    return 0;
}

int
obd_send(int fd, float val, char *format)
{
    char buf[256];
    char buf2[256];
    
    // check if value was set
    if (val == -2)
        return -1;
    
    snprintf (buf, sizeof (buf), format, val);
    snprintf (buf2, sizeof (buf2), "Content-Length: %d\r\n", strlen(buf));
    send (fd, HTTP_OK, strlen(HTTP_OK), 0);
    send (fd, buf2, strlen(buf2), 0);
    send (fd, HEADER_PLAIN, strlen(HEADER_PLAIN), 0);
    send (fd, buf, strlen (buf), 0);    

    return 0;
}


int
handle_client(int fd)
{
    
    int file_fd;
    int r, i, j;
    static char buffer[1024];
    char out[1024];
    struct stat stats;
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

    //printf("request: %s\n", buffer);
    
    
    if (strncmp(buffer,"GET ", 4) && strncmp(buffer,"POST ", 5) )
    {
        printf("we only support POST and GET but got: %s\n", buffer);
        return -1;
    }
    
    // look for second space and terminate string (skip headers)
    if ( (p = strchr(buffer, ' ')) != NULL)
        if ( (p = strchr(p+1, ' ')) != NULL)
            *p = '\0';
    
    
    // check for illegal parent directory requests
    for (j = 0; j < i - 1; j++)
    {
        if(buffer[j] == '.' && buffer[j + 1] == '.')
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
            obd_send(fd, gval->speed, "%.01f");
        else if (!strcmp(p, "rpm") )
            obd_send(fd, gval->rpm, "%.00f");
        else if (!strcmp(p, "con_h") )
            obd_send(fd, gval->con_h, "%.02f");
        else if (!strcmp(p, "con_km") )
            obd_send(fd, gval->con_km, "%.02f");
        
        else if (!strcmp(p, "con_av_short") )
            obd_send(fd, av_con->average_short, "%.02f");
        else if (!strcmp(p, "con_av_medium") )
            obd_send(fd, av_con->average_medium, "%.02f");
        else if (!strcmp(p, "con_av_long") )
            obd_send(fd, av_con->average_long, "%.02f");
        
        else if (!strcmp(p, "speed_av_short") )
            obd_send(fd, av_speed->average_short, "%.02f");
        else if (!strcmp(p, "speed_av_medium") )
            obd_send(fd, av_speed->average_medium, "%.02f");
        else if (!strcmp(p, "speed_av_long") )
            obd_send(fd, av_speed->average_long, "%.02f");
        
        else if (!strcmp(p, "temp1") )
            obd_send(fd, gval->temp1, "%.01f");
        else if (!strcmp(p, "temp2") )
            obd_send(fd, gval->temp2, "%.01f");
        else if (!strcmp(p, "voltage") )
            obd_send(fd, gval->voltage, "%.02f");
        else if (!strcmp(p, "liters") )
            obd_send(fd, av_con->liters, "%.03f");


        else if (!strcmp(p, "debug") )
            obd_send_debug(fd);        
        
        else if (!strcmp(p, "reset") )
            reset_counters();
        
        else if (!strcmp(p, "av_speed_graph:short") )
            av_speed->timespan = 300;
        else if (!strcmp(p, "av_speed_graph:medium") )
            av_speed->timespan = 1800;
        else if (!strcmp(p, "av_speed_graph:long") )
            av_speed->timespan = 14400;
        
        else if (!strcmp(p, "av_con_graph:short") )
            av_con->timespan = 300;            
        else if (!strcmp(p, "av_con_graph:medium") )
            av_con->timespan = 1800;            
        else if (!strcmp(p, "av_con_graph:long") )            
            av_con->timespan = 14400;
        
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


    if (stat(&buffer[5], &stats) == -1)
    {
#ifdef DEBUG        
        printf("file with 0bytes: %s\n", &buffer[0]);
#endif        
        return -1;
    }
    
    /*
    i = 0;
    
    // if filesize == 0, wait and try again, max 5 times
    do {
        if (stat(&buffer[5], &stats) == -1)
        {
            perror("stat()");
            return -1;
        }
        
        usleep(50000);
        i++;
        
        if ( i > 30)
        {
            printf("file with 0bytes: %s\n", &buffer[0]);
            return -1;
        }
        
    } while (stats.st_size == 0);
    */
    
    
    // send content length
    snprintf(out, sizeof(out), HTTP_OK
             "Content-Length: %jd\r\n", (intmax_t) stats.st_size);
    write(fd, out, strlen(out));
    
#ifdef DEBUG
    printf("sending file: %s with %9jd length\n", &buffer[5], (intmax_t) stats.st_size);
#endif
    
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
    
    
    // open and send file
    if(( file_fd = open(&buffer[5],O_RDONLY)) == -1)
    {
        perror("open()");
        return -1;
    }
    
    while ( (r = read(file_fd, buffer, sizeof(buffer))) > 0 )
        write(fd, buffer,r);

    close(file_fd);
    
    return 0;
}


void
ajax_log(char *s)
{
    printf("%s", s);
    snprintf(debug, 1024, "%s", s);
    return;   
}


void
reset_counters(void)
{

#ifdef DEBUG 
    printf("resetting counters...\n");
#endif    

    // init average consumption struct
    av_con->array_full = 0;
    av_con->counter = 0;
    av_con->average_short = 0; 
    av_con->average_medium = 0;
    av_con->average_long = 0;
    
    // init average speed struct
    av_speed->array_full = 0;
    av_speed->counter = 0;
    av_speed->average_short = 0; 
    av_speed->average_medium = 0;
    av_speed->average_long = 0;
    
#ifdef DEBUG
    sleep(1);
    printf("values: %.02f %d %d %.02f\n", 
           av_con->average_short, av_con->counter, av_speed->counter, av_speed->average_long);
#endif
    
    return;
}
