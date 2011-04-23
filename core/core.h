// TODO: check whether terminal/socket includes can be in the file that needs them only
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <errno.h>
#include <signal.h>

// tcp sockets
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// for priority settings
#include <sched.h>

// include common defines
#include "../common/common.h"

// the engine data struct
typedef struct engine_data
{
    float rpm;
    float injection_time;
    float oil_pressure;
    float speed;

    // consumption (will be calculated)
    float per_h;
    float per_km;
} engine_data;

// struct for other data
typedef struct other_data
{
    float temp_engine;
    float temp_air_intake;
    float voltage;
} other_data;


int  kw1281_open (char *device);
int  kw1281_close(void);
int  kw1281_init (int);
int  kw1281_mainloop (void);

void handle_data(char *, float);

void db_send_engine_data(engine_data);
void db_send_other_data(other_data);
