#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>



#define SERIAL_ATTACHED
//#define DEBUG

#define DEVICE          "/dev/ttyUSB0"
#define BAUDRATE        10400
// serial dump logs 15600ms before answer arrives maybe 7800 is better?
#define WRITE_DELAY     5700
#define INIT_DELAY      200000

#define PORT            8080

// measure length in seconds (for calculating averages)
#define SHORT       300     // last 5 minutes
#define MEDIUM      1800    // last 30 minutes
#define LONG        14400   // last 4 hours

#define CON_AV_FILE     "consumption.data"
#define SPEED_AV_FILE   "speed.data"

void    rrdtool_create_speed (void);
void    rrdtool_create_consumption (void);
void    rrdtool_update_speed (void);
void    rrdtool_update_consumption (void);

int     kw1281_open (char *device);
int     kw1281_init (int);
int     kw1281_mainloop (void);

void    ajax_log(char *s);
void   *ajax_socket (void *);
int     ajax_shutdown(void);

float   speed, rpm, temp1, temp2, oil_press, inj_time, voltage;
float   con_h;
float   con_km;

// averages
struct average
{
    float   array[LONG];
    char    array_full;
    int     counter;
    float   average_short;  // average of short time period (SHORT)
    float   average_medium; // average of medium time period (MEDIUM)
    float   average_long;   // average of long time period (LONG)  
} av_con, av_speed;


char    debug[1024];    // debuging messages go here

// make server socket global so we can close/shutdown it on exit
int     srv;
