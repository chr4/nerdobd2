#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
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



//#define SERIAL_ATTACHED
//#define DEBUG

#define DEVICE          "/dev/ttyUSB0"
#define BAUDRATE        10400
#define WRITE_DELAY     5700
#define INIT_DELAY      200000

#define PORT		8080

void    rrdtool_create_speed (void);
void    rrdtool_create_consumption (void);
void    rrdtool_update_speed (void);
void    rrdtool_update_consumption (void);

int     kw1281_open (char *device);
int     kw1281_init (int);
int     kw1281_mainloop (void);

void   *ajax_socket (void *);
int     ajax_shutdown(void);

float   speed, rpm, temp1, temp2, oil_press, inj_time, load, voltage;
float   con_h;
float   con_km;

// make server socket global so we can close/shutdown it on exit
int     srv;
