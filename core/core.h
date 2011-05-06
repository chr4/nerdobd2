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
#include <sys/time.h>

// include common defines
#include "../common/common.h"
#include "../common/config.h"

// we need sqlite3 access
#include "../common/sqlite.h"


#define SERIAL_HARD_ERROR   -2
#define SERIAL_SOFT_ERROR   -1


// the engine data struct
typedef struct engine_data
{
    float rpm;
    float injection_time;
    float oil_pressure;
    float speed;

    // consumption (will be calculated)
    float consumption_per_h;
    float consumption_per_100km;
    
    float duration_consumption;
    float duration_speed;
    
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

void handle_data(char *, float, float);
