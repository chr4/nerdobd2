#include "httpd.h"
#include "tcp.h"

#define SERVER_STRING   "Server: nerdobd ajax server |0.9.4\r\n"
#define SERVER_CON      "Connection: close\r\n"
#define HTTP_OK         "HTTP/1.0 200 OK\r\n"
#define HTTP_ERROR      "HTTP/1.0 404 Not Found\r\n"

#define HEADER_PLAIN    SERVER_STRING SERVER_CON "Content-Type: text/plain\r\n\r\n"
#define HEADER_HTML     SERVER_STRING SERVER_CON "Content-Type: text/html\r\n\r\n"
#define HEADER_PNG      SERVER_STRING SERVER_CON "Content-Type: image/png\r\n\r\n"
#define HEADER_CSS      SERVER_STRING SERVER_CON "Content-Type: text/css\r\n\r\n"
#define HEADER_JS       SERVER_STRING SERVER_CON "Content-Type: application/x-javascript\r\n\r\n"
#define HEADER_ICON     SERVER_STRING SERVER_CON "Content-Type: image/x-icon\r\n\r\n"
#define HEADER_TTF      SERVER_STRING SERVER_CON "Content-Type: font/ttf\r\n\r\n"

#ifdef DB_SQLITE
sqlite3 *db;
#endif

#ifdef DB_POSTGRES
PGconn *db;
#endif

int
send_error(int fd, char *message)
{
    char out[LEN_BUFFER];

    snprintf(out, sizeof(out), HTTP_ERROR
             "Content-Length: %d\r\n", strlen(message));

    if (write(fd, out, strlen(out)) <= 0)
        return -1;

    if (write(fd, HEADER_PLAIN, strlen(HEADER_PLAIN)) <= 0)
        return -1;

    if (write(fd, message, strlen(message)) <= 0)
        return -1;

    return 0;
}


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
    if (write(fd, out, strlen(out)) <= 0)
        return -1;


#ifdef DEBUG_AJAX
    printf("sending file: %s with %9jd length\n", path, (intmax_t) stats.st_size);
#endif

    // is file type known?
    if ( !strcmp(p, ".html") ||  !strcmp(p, ".htm") )
    {
        if (write(fd, HEADER_HTML, strlen(HEADER_HTML)) <= 0)
            return -1;
    }
    else if (!strcmp(p, ".png") )
    {
        if (write(fd, HEADER_PNG, strlen(HEADER_PNG)) <= 0)
            return -1;
    }
    else if (!strcmp(p, ".txt") )
    {
        if (write(fd, HEADER_PLAIN, strlen(HEADER_PLAIN)) <= 0)
            return -1;
    }
    else if (!strcmp(p, ".js") )
    {
        if (write(fd, HEADER_JS, strlen(HEADER_JS)) <= 0)
            return -1;
    }
    else if (!strcmp(p, ".css") )
    {
        if (write(fd, HEADER_CSS, strlen(HEADER_CSS)) <= 0)
            return -1;
    }
    else if (!strcmp(p, ".ico") )
    {
        if (write(fd, HEADER_ICON, strlen(HEADER_ICON)) <= 0)
            return -1;
    }
    else if (!strcmp(p, ".ttf") )
    {
        if (write(fd, HEADER_TTF, strlen(HEADER_TTF)) <= 0)
            return -1;
    }
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
        if (write(fd, out, r) <= 0)
            return -1;

    close(file_fd);
    return 0;
}

int
send_json(int fd, const char *json)
{
    char out[LEN_BUFFER];

    // send content length
    snprintf(out, sizeof(out), HTTP_OK
             "Content-Length: %d\r\n", strlen(json));

#ifdef DEBUG_AJAX
    // printf("serving json:\n%s\n", json);
#endif

    if (write(fd, out, strlen(out)) <= 0)
        return -1;

    if (write(fd, HEADER_PLAIN, strlen(HEADER_PLAIN)) <= 0)
        return -1;

    if (write(fd, json, strlen(json)) <= 0)
        return -1;

    return 0;
}


int
send_latest_data(int fd)
{
    const char *json;

    json = json_latest_data(db);

    if (send_json(fd, json) == -1)
        return -1;

    return 0;
}


int
send_graph_data(int fd, char *graph, char *args)
{
    char       *p;
    const char *json;
    long        index = 0;
    long        timespan = 300;

    // parse arguments
    if (strtok(args, "?") != NULL)
    {
        p = strtok(NULL, "=");
        while (p != NULL)
        {
            if (!strcmp(p, "index"))
                index = atoi(strtok(NULL, "&"));
            else if (!strcmp(p, "timespan"))
                timespan = atoi(strtok(NULL, "&"));

            p = strtok(NULL, "=");
        }
    }

    json = json_graph_data(db, graph, index, timespan);

    if (send_json(fd, json) == -1)
        return -1;

    return 0;
}


void
handle_client(int fd)
{
    int r;
    int i;
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


    // filter requests we don't support
    if (strncmp(buffer,"GET ", 4) && strncmp(buffer,"POST ", 5) )
    {
        send_error(fd, "not supported (only GET and POST)");
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
        send_error(fd, "invalid request.\n");
        return;
    }

    // convert / to index.html
    if (!strncmp(buffer, "GET /\0", 6))
        strncpy(buffer, "GET /index.html", sizeof(buffer));


    // check for illegal parent directory requests
    for (i = 0; ; i++)
    {
        if (buffer[i] == '\0' || buffer[i + 1] == '\0')
            break;

        if(buffer[i] == '.' && buffer[i + 1] == '.')
        {
            send_error(fd, ".. detected.\n");
            return;
        }
    }

    // point p to filename
    if ( (p = strchr(buffer, '/')) == NULL)
        return;

#ifdef DEBUG_AJAX
    printf("requested: %s\n", p);
#endif

    // send json data
    if (!strncmp(p, "/data.json", 10) )
        send_latest_data(fd);
    else if (!strncmp(p, "/consumption.json", 17) )
        send_graph_data(fd, "consumption_per_100km", buffer);
    else if (!strncmp(p, "/speed.json", 11) )
        send_graph_data(fd, "speed", buffer);

    // send file
    else
        if (send_file(fd, p) != 0)
            send_error(fd, "could not send file.\n");

    return;
}

void
httpd_stop(int signo)
{
    printf(" - child (httpd): closing connections...\n");
    do {
        wait(NULL);
    } while (errno != ECHILD);

    printf(" - child (httpd): closing database...\n");
    close_db(db);

    _exit(0);
}

int
httpd_start(void)
{
    int s;
    pid_t pid;

    // open the database
    if ( (db = open_db()) == NULL)
        return -1;

    if ( (s = tcp_listen(HTTPD_PORT)) == -1)
        return -1;

    if ( (pid = fork()) == 0)
    {
        // add signal handler for cleanup function
        signal(SIGINT, httpd_stop);
        signal(SIGTERM, httpd_stop);

        tcp_loop_accept(s, &handle_client);

        // should never be reached
        _exit(0);
    }

    // we don't need the socket in this process
    close(s);
    return pid;
}

