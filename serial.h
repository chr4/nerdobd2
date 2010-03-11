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

// for priority settings
#include <sched.h>

// shm includes
#include <sys/ipc.h>
#include <sys/shm.h>

// include rrd stuff (librrd-dev)
#include <rrd.h>


#define SERIAL_ATTACHED
#define HIGH_PRIORITY
//#define DEBUG

#define DEVICE          "/dev/ttyUSB0"
#define BAUDRATE        10400
#define WRITE_DELAY     5700    // serial dump logs 15600ms before answer arrives maybe 7800 is better?
#define INIT_DELAY      200000

#define PORT            8080

// measure length in seconds (for calculating averages)
#define SHORT           300     // last 5 minutes
#define MEDIUM          1800    // last 30 minutes
#define LONG            14400   // last 4 hours

// saving on ramdisk
#define CON_AV_FILE     "/tmp/nerdobd2_consumption.data"
#define SPEED_AV_FILE   "/tmp/nerdobd2_speed.data"
#define SPEED_GRAPH     "/tmp/nerdobd2_speed.png"
#define CON_GRAPH       "/tmp/nerdobd2_consumption.png"

void    rrdtool_create_speed (void);
void    rrdtool_create_consumption (void);
void    rrdtool_update_speed (void);
void    rrdtool_update_consumption (void);

int     kw1281_open (char *device);
int     kw1281_close(void);
int     kw1281_init (int);
int     kw1281_mainloop (void);

void    ajax_log(char *s);
void    ajax_socket(int);
int     ajax_shutdown(void);


// global values
struct values
{
    // values from obd2
    float   speed;
    float   rpm;
    float   temp1;
    float   temp2;
    float   oil_press;
    float   inj_time;
    float   voltage;
    
    // calculated values
    float   con_h;
    float   con_km;

    // time span for rrdtool graph
    int     con_timespan;
    int     speed_timespan;
};

struct values *gval;


// averages
struct average
{
    float   array[LONG];
    char    array_full;
    int     counter;
    float   average_short;  // average of short time period (SHORT)
    float   average_medium; // average of medium time period (MEDIUM)
    float   average_long;   // average of long time period (LONG)
    
};

struct average *av_con, *av_speed;


// debuging messages go here
char    *debug; 

// make server socket global so we can close/shutdown it on exit
int     srv;
