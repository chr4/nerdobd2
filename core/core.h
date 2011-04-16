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

// unix domain sockets
#include <sys/socket.h>
#include <sys/un.h>

// for priority settings
#include <sched.h>

int     kw1281_open (char *device);
int     kw1281_close(void);
int     kw1281_init (int);
int     kw1281_mainloop (void);

int     db_send(char *, float);
