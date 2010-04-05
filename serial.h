#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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


#define SERIAL_ATTACHED   // define if serial cable is attached (for debugging, comment out)
#define HIGH_PRIORITY       // whether to run program with high priority (for more reliable serial communication)
//#define DEBUG

#define DEVICE          "/dev/ttyUSB0"
#define BAUDRATE        10400   // for my seat arosa, vw polo needs 9600
#define WRITE_DELAY     5700    // serial dump logs 15600ms before answer arrives maybe 7800 is better?
#define INIT_DELAY      200000  // nanosec to wait to emulate 5baud

/* constants for calculating consumption
 * out of injection time, rpm and speed
 *
 * consumption per hour = 
 *    60 (minutes) * 4 (zylinders) * 
 *    MULTIPLIER * rpm * (injection time - INJ_SUBTRACT)
 */
#define MULTIPLIER      0.00000089  // for my 1.0l seat arosa 2004
#define INJ_SUBTRACT    0.1

// the port where to local ajax server listens
#define PORT            8080

// measure length in seconds (for calculating averages)
#define SHORT           300     // last 5 minutes
#define MEDIUM          1800    // last 30 minutes
#define LONG            14400   // last 4 hours


#define TANK_CONT_MAX_TRYS 5    // try to get tank content up to 5 times
#define TANK_REQUEST    3       // return value

// saving on ramdisk
#define CON_AV_FILE     "consumption.data"
#define SPEED_AV_FILE   "speed.data"
#define SPEED_GRAPH     "speed.png"
#define CON_GRAPH       "consumption.png"

void    rrdtool_create_speed (void);
void    rrdtool_create_consumption (void);
void    rrdtool_update_speed (void);
void    rrdtool_update_consumption (void);

int     kw1281_open (char *device);
int     kw1281_close(void);
int     kw1281_init (int);
int     kw1281_get_tank_cont(void);
int     kw1281_mainloop (void);

void    ajax_log(char *s);
void    ajax_socket(int);


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
    float   tank;
    char    tank_request;   // flag for requesting tank content
    
    // calculated values
    float   con_h;
    float   con_km; 
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

    int     timespan;       // time span for rrdtool graph
    
    //float   liters;         // only av_con uses this, absolute consumption
};

struct average *av_con, *av_speed;


// debuging messages go here
char    *debug; 

// make server socket global so we can close/shutdown it on exit
int     srv;
