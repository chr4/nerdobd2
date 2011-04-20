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

// unix domain sockets
#include <sys/socket.h>
#include <sys/un.h>

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


int  kw1281_open (char *device);
int  kw1281_close(void);
int  kw1281_init (int);
int  kw1281_mainloop (void);

void handle_data(char *, float);

void db_send_engine_data(engine_data);
void db_send_other_data(char *, float);
