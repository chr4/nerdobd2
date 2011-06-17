#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>

#define SERIAL_HARD_ERROR   -2
#define SERIAL_SOFT_ERROR   -1


int  kw1281_open (char *device);
int  kw1281_close(void);
int  kw1281_init (int, int);
int  kw1281_mainloop (void);

// flags if this is the first run
char consumption_first_run;
char speed_first_run;

