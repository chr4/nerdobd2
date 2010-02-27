#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>
#include <linux/termios.h>
#include <linux/serial.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <pthread.h>


// #define DEBUG

int     kw1281_open (char *device);
void    kw1281_init (int);
void    kw1281_mainloop (void);

int     ajax_socket (int);


float   speed, rpm, temp1, temp2, oil_press, inj_time, load, voltage;
float   l_per_h;
float   l_per_100km;