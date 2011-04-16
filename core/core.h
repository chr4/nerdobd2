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

// for priority settings
#include <sched.h>

#define HIGH_PRIORITY       // whether to run program with high priority (for more reliable serial communication)

#define DEBUG_SERIAL

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

int     kw1281_open (char *device);
int     kw1281_close(void);
int     kw1281_init (int);
int     kw1281_mainloop (void);
