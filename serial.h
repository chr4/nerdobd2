#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
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
#include <pthread.h>


#define SERIAL_ATTACHED
#define DEBUG

#define DEVICE		"/dev/ttyUSB0"
#define BAUDRATE        10400
#define WRITE_DELAY     5700
#define INIT_DELAY      200000

void    rrdtool_create_speed (void);
void    rrdtool_create_consumption (void);

int     kw1281_open (char *device);
void    kw1281_init (int);
void   *kw1281_mainloop ();

void   *ajax_socket (void *);


float   speed, rpm, temp1, temp2, oil_press, inj_time, load, voltage;
float   con_h;
float   con_km;
