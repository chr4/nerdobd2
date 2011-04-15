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

// sqlite database
#include <sqlite3.h>

// for priority settings
#include <sched.h>

// shm includes
#include <sys/ipc.h>
#include <sys/shm.h>

// include rrd stuff (librrd-dev)
#include <rrd.h>


//#define SERIAL_ATTACHED   // define if serial cable is attached (for debugging, comment out)
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

#define TANK_CONT_MAX_TRYS 5    // try to get tank content up to 5 times
#define TANK_REQUEST    3       // return value

#define SPEED_GRAPH     "speed.png"
#define CON_GRAPH       "consumption.png"

// the database file (sqlite3) and file handler
#define DB_FILE         "database.sqlite3"
sqlite3 *db;

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

// sqlite functions
int     exec_query(char *);
int     create_table(char *);
int     insert_value(char *, float);
float   get_value(char *);
float   get_row(char *, char *);
float   get_average(char*, char *, int);
int     calc_consumption(void);
int     init_db(void);
void    close_db(void);


// debuging messages go here
char    *debug; 

// make server socket global so we can close/shutdown it on exit
int     srv;

// flag for tank request
char    tank_request;

int	speed_graph_timespan;
int	consumption_graph_timespan;
