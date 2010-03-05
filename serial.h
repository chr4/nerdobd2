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
#define WRITE_DELAY     5700
#define INIT_DELAY      200000

#define PORT            8080

// consumption measure length in seconds
#define CON_SHORT       300     // last 5 minutes
#define CON_MEDIUM      1800    // last 30 minutes
#define CON_LONG        14400   // last 4 hours

#define CON_AV_FILE     "consumption.data"

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

float   speed, rpm, temp1, temp2, oil_press, inj_time, load, voltage;
float   con_h;
float   con_km;

// average consumption
struct con_av
{
    float   array[CON_LONG];
    char    array_full;
    int     counter;
    float   average_short;
    float   average_medium;
    float   average_long;    
} consumption;


char    debug[1024];    // debuging messages go here

// make server socket global so we can close/shutdown it on exit
int     srv;
