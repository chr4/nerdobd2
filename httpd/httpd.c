#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

// for file operations
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../common/common.h"
#include "../common/config.h"
#include "../common/tcp.h"

#define SERVER_STRING   "Server: nerdobd ajax server |0.9.4\r\n"
#define SERVER_CON      "Connection: close\r\n"
#define HTTP_OK         "HTTP/1.0 200 OK\r\n"

#define HEADER_PLAIN    SERVER_STRING SERVER_CON "Content-Type: text/plain\r\n\r\n"
#define HEADER_HTML     SERVER_STRING SERVER_CON "Content-Type: text/html\r\n\r\n"
#define HEADER_PNG      SERVER_STRING SERVER_CON "Content-Type: image/png\r\n\r\n"
#define HEADER_CSS      SERVER_STRING SERVER_CON "Content-Type: text/css\r\n\r\n"
#define HEADER_JS       SERVER_STRING SERVER_CON "Content-Type: application/x-javascript\r\n\r\n"
#define HEADER_ICON     SERVER_STRING SERVER_CON "Content-Type: image/x-icon\r\n\r\n"



int
send_file(int fd, char *filename)
{
    int file_fd;
    int r;
    char *p;
    char out[LEN_BUFFER];
    struct stat stats;
    char path[LEN];
    
    
    // terminate arguments after ?
    if ( (p = strchr(filename, '?')) != NULL) 
        *p = '\0';

    // search for last dot in filename
    if ( (p = strrchr(filename, '.')) == NULL)
    {
        printf("no . found in filename!\n");
        return -1;
    }
    
    // merge filename with docroot path
    snprintf(path, sizeof(path), "%s%s", DOCROOT, filename);

    if (stat(path, &stats) == -1)
    {
#ifdef DEBUG_AJAX        
        printf("file with 0bytes: %s\n", path);
#endif        
        return -1;
    }
    
    
    // send content length
    snprintf(out, sizeof(out), HTTP_OK
             "Content-Length: %jd\r\n", (intmax_t) stats.st_size);
    write(fd, out, strlen(out));
    
#ifdef DEBUG_AJAX
    printf("sending file: %s with %9jd length\n", path, (intmax_t) stats.st_size);
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
    if(( file_fd = open(path, O_RDONLY)) == -1)
    {
        perror("open()");
        return -1;
    }
    
    while ( (r = read(file_fd, out, sizeof(out))) > 0 )
        write(fd, out, r);
    
    close(file_fd);
    return 0;
}


int
send_json_data(int fd, char *args)
{  
    char *p, *q;
    char out[LEN_BUFFER];
    char json[LEN_JSON];
    int speed_index, consumption_index, timespan;
    
    // parse arguments
    // if no arguments are given, set args to 0
    if ( (p = strchr(args, '?')) == NULL)
    {
        speed_index = 0;
        consumption_index = 0;
        timespan = 0;
    }
    
    // parse arguments
    else
    {
        q = strtok(p + 1, "=");
        while (q != NULL)
        {
            if (!strcmp(q, "speed_index"))
                speed_index = atoi(strtok(NULL, "&"));
            else if (!strcmp(q, "consumption_index"))
                consumption_index = atoi(strtok(NULL, "&"));
            else if (!strcmp(q, "timespan"))
                timespan = atoi(strtok(NULL, "&"));
            
            q = strtok(NULL, "=");
        }
    }
    
    //strncpy(json, json_generate(consumption_index, speed_index, timespan), sizeof(json));
    strcpy(json, "test");
    
#ifdef DEBUG_AJAX 
    printf("serving json:\n%s\n", json);
#endif
    
    // send content length
    snprintf(out, sizeof(out), HTTP_OK
             "Content-Length: %jd\r\n", (intmax_t) strlen(json));
    
    write(fd, out, strlen(out));
    write(fd, HEADER_PLAIN, strlen(HEADER_PLAIN));
    write(fd, json, strlen(json));
    
    return 0;
}


void
handle_client(int fd)
{
    int r;
    int i, j;
    char *p;
    static char buffer[LEN_BUFFER];
    
    
    // read socket
    if ( (r = read (fd, buffer, sizeof(buffer))) < 0)
    {
        printf("read() failed\n");
        return;
    }
    
    // terminate received string
    if ( r > 0 && r < sizeof(buffer))
        buffer[r] = '\0';
    else
        buffer[0] = '\0';
    
    printf("buffer: %s\n", buffer);

    // filter requests we don't support
    if (strncmp(buffer,"GET ", 4) && strncmp(buffer,"POST ", 5) )
    {
        printf("not supported: %s\n", buffer);
        return;
    }
    
    // look for second space (or newline) and terminate string (skip headers)
    if ( (p = strchr(buffer, ' ')) != NULL)
    {
        for ( ; ; )
        {
            p++;
            if(*p == '\r' || *p == '\n' || *p == ' ')
            {
                *p = '\0';
                break;
            }
        }
    }
    else
    {
        printf("no space found\n");
        return;
    }
    
    // convert / to index.html
    if (!strncmp(buffer, "GET /\0", 6)) 
        strncpy(buffer, "GET /index.html", sizeof(buffer));
    
    
    // check for illegal parent directory requests
    for (j = 0; j < i - 1; j++)
    {
        if(buffer[j] == '.' && buffer[j + 1] == '.')
        {
            printf(".. detected\n");
            return;
        }
    }

    // point p to filename
    if ( (p = strchr(buffer, '/')) == NULL)
        return;
    
    
    // send json data
    if (!strncmp(p, "/data.json", 14) )
        send_json_data(fd, buffer);
 
    // send file
    else
        if (send_file(fd, p) != 0)
            printf("couldn't send file: %s\n", p);
    
    return;
}


int
main(int argc, char **argv)
{
    int s;
    
    if ( (s = tcp_listen(HTTPD_PORT)) == -1)
        return -1;
    
    tcp_loop_accept(s, &handle_client);
    
    // should never be reached
    return 0;
}
