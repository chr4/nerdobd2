#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>


// including ioctl.h conflicts with terminos.h
// #include <sys/ioctl.h>

#include <linux/termios.h>
#include <linux/serial.h>

#define WRITE_DELAY        5700
#define INIT_DELAY        200000

//#define DEBUG

static void _set_bit (int);

int     kw1281_setup (char *device);
void    kw1281_init (int);
void    kw1281_mainloop (void);
void    kw1281_handle_error (void);
void    kw1281_send_byte_ack (unsigned char);
void    kw1281_send_ack (void);
void    kw1281_send_block (unsigned char);
void    kw1281_recv_block (unsigned char);
int     kw1281_inc_counter ();
unsigned char kw1281_recv_byte_ack (void);

int     tcp_listen (int);
int     ajax_socket (int);
int     handle_client (int);
void    cut_crlf (char *);
ssize_t readline (int, void *, size_t);
char   *get_line (FILE *);
FILE   *open_html (char *);

void    rrdtool_update (char *, float);
void    output_values (void);

int     fd;
int     counter;		/* kw1281 protocol block counter */
int     ready = 0;

float   speed, rpm, temp1, temp2, oil_press, inj_time, load, voltage;
float   const_multiplier = 0.00000089;
float   const_inj_subtract = 0.1;
float   l_per_h;
float   l_per_100km;

/* execute the rddtool command
 * name = rdd file to write val to
 * val = value
 */
void
rrdtool_update (char *name, float val)
{
    pid_t   pid;
    time_t  t;
    int     status;
    char    cmd[256];

    if ((pid = fork ()) == 0)
    {
	snprintf (cmd, sizeof (cmd), "%d:%.2f", (int) time (&t), val);

	if (execlp ("rrdtool", "rrdtool", "update", name, cmd, NULL) == -1)
	    exit (-1);
    }

    if (pid < 0)
	return;

    waitpid (pid, &status, 0);

    return;
}

/* manually set serial lines */
static void
_set_bit (int bit)
{
    int     flags;

    ioctl (fd, TIOCMGET, &flags);

    if (bit)
    {
	ioctl (fd, TIOCCBRK, 0);
	flags &= ~TIOCM_RTS;
    }
    else
    {
	ioctl (fd, TIOCSBRK, 0);
	flags |= TIOCM_RTS;
    }

    ioctl (fd, TIOCMSET, &flags);
}

/* function in case an error occures */
void
kw1281_handle_error (void)
{
    /*
     * recv() until 0x8a
     * then send 0x75 (complement)
     * reset counter = 1
     * continue with block readings
     *
     *  or just exit -1 and start program in a loop
     */

    exit (-1);
}

// increment the counter
int
kw1281_inc_counter (void)
{
    if (counter == 255)
    {
	counter = 1;
	return 255;
    }
    else
	counter++;

    return counter - 1;
}

/* receive one byte and acknowledge it */
unsigned char
kw1281_recv_byte_ack (void)
{
    unsigned char c, d;

    read (fd, &c, 1);
    d = 0xff - c;
    usleep (WRITE_DELAY);
    write (fd, &d, 1);
    read (fd, &d, 1);
    if (0xff - c != d)
    {
	printf ("kw1281_recv_byte_ack: echo error recv: 0x%02x (!= 0x%02x)\n",
		d, 0xff - c);

	kw1281_handle_error ();
    }
    return c;
}

/* send one byte and wait for acknowledgement */
void
kw1281_send_byte_ack (unsigned char c)
{
    unsigned char d;

    usleep (WRITE_DELAY);
    write (fd, &c, 1);
    read (fd, &d, 1);
    if (c != d)
    {
	printf ("kw1281_send_byte_ack: echo error (0x%02x != 0x%02x)\n", c,
		d);
	kw1281_handle_error ();
    }

    read (fd, &d, 1);
    if (0xff - c != d)
    {
	printf ("kw1281_send_byte_ack: ack error (0x%02x != 0x%02x)\n",
		0xff - c, d);
	kw1281_handle_error ();
    }
}

/* write 7O1 address byte at 5 baud and wait for sync/keyword bytes */
void
kw1281_init (int address)
{
    int     i, p, flags;
    unsigned char c;

    int     in;

    /* prepare to send (clear dtr and rts) */
    ioctl (fd, TIOCMGET, &flags);
    flags &= ~(TIOCM_DTR | TIOCM_RTS);
    ioctl (fd, TIOCMSET, &flags);
    usleep (INIT_DELAY);

    _set_bit (0);		/* start bit */
    usleep (INIT_DELAY);	/* 5 baud */
    p = 1;
    for (i = 0; i < 7; i++)
    {				/* address bits, lsb first */
	int     bit = (address >> i) & 0x1;
	_set_bit (bit);
	p = p ^ bit;
	usleep (INIT_DELAY);
    }
    _set_bit (p);		/* odd parity */
    usleep (INIT_DELAY);
    _set_bit (1);		/* stop bit */
    usleep (INIT_DELAY);

    /* set dtr */
    ioctl (fd, TIOCMGET, &flags);
    flags |= TIOCM_DTR;
    ioctl (fd, TIOCMSET, &flags);

    /* read bogus values, if any */
    ioctl (fd, FIONREAD, &in);
    while (in--)
    {
	read (fd, &c, 1);
#ifdef DEBUG
	printf ("ignore 0x%02x\n", c);
#endif
    }

    read (fd, &c, 1);
#ifdef DEBUG
    printf ("read 0x%02x\n", c);
#endif

    read (fd, &c, 1);
#ifdef DEBUG
    printf ("read 0x%02x\n", c);
#endif

    c = kw1281_recv_byte_ack ();
#ifdef DEBUG
    printf ("read 0x%02x (and sent ack)\n", c);
#endif

    counter = 1;

    return;
}

/* send an ACK block */
void
kw1281_send_ack ()
{
    unsigned char c;

#ifdef DEBUG
    printf ("send ACK block %d\n", counter);
#endif

    /* block length */
    kw1281_send_byte_ack (0x03);

    kw1281_send_byte_ack (kw1281_inc_counter ());

    /* ack command */
    kw1281_send_byte_ack (0x09);

    /* block end */
    c = 0x03;
    usleep (WRITE_DELAY);
    write (fd, &c, 1);
    read (fd, &c, 1);
    if (c != 0x03)
    {
	printf ("echo error (0x03 != 0x%02x)\n", c);
	kw1281_handle_error ();
    }

    return;
}

/* send group reading block */
void
kw1281_send_block (unsigned char n)
{
    unsigned char c;

#ifdef DEBUG
    printf ("send group reading block %d\n", counter);
#endif

    /* block length */
    kw1281_send_byte_ack (0x04);

    // counter
    kw1281_send_byte_ack (kw1281_inc_counter ());

    /*  type group reading */
    kw1281_send_byte_ack (0x29);

    /* which group block */
    kw1281_send_byte_ack (n);

    /* block end */
    c = 0x03;
    usleep (WRITE_DELAY);
    write (fd, &c, 1);
    read (fd, &c, 1);
    if (c != 0x03)
    {
	printf ("echo error (0x03 != 0x%02x)\n", c);
	kw1281_handle_error ();
    }
    return;
}

/* receive a complete block */
void
kw1281_recv_block (unsigned char n)
{
    int     i;
    unsigned char c, l, t;
    unsigned char buf[256];

    /* block length */
    l = kw1281_recv_byte_ack ();

    c = kw1281_recv_byte_ack ();

    if (c != counter)
    {
	printf ("counter error (%d != %d)\n", counter, c);

#ifdef DEBUG
	printf ("IN   OUT\t(block dump)\n");
	printf ("0x%02x\t\t(block length)\n", l);
	printf ("     0x%02x\t(ack)\n", 0xff - l);
	printf ("0x%02x\t\t(counter)\n", c);
	printf ("     0x%02x\t(ack)\n", 0xff - c);
	/*
	   while (1) {
	   c = kw1281_recv_byte_ack();
	   printf("0x%02x\t\t(data)\n", c);
	   printf("     0x%02x\t(ack)\n", 0xff - c);
	   }
	 */
#endif

	kw1281_handle_error ();
    }

    t = kw1281_recv_byte_ack ();

#ifdef DEBUG
    switch (t)
    {
	case 0xf6:
	    printf ("got ASCII block %d\n", counter);
	    break;
	case 0x09:
	    printf ("got ACK block %d\n", counter);
	    break;
	case 0x00:
	    printf ("got 0x00 block %d\n", counter);
	case 0xe7:
	    printf ("got group reading answer block %d\n", counter);
	    break;
	default:
	    printf ("block title: 0x%02x (block %d)\n", t, counter);
	    break;
    }
#endif

    l -= 2;

    i = 0;
    while (--l)
    {
	c = kw1281_recv_byte_ack ();

	buf[i++] = c;

#ifdef DEBUG
	printf ("0x%02x ", c);
#endif

    }
    buf[i] = 0;

#ifdef DEBUG
    if (t == 0xf6)
	printf ("= \"%s\"\n", buf);
#endif

    if (t == 0xe7)
    {

	// look at field headers 0, 3, 6, 9
	for (i = 0; i <= 9; i += 3)
	{
	    switch (buf[i])
	    {
		case 0x01:	// rpm
		    if (i == 0)
			rpm = 0.2 * buf[i + 1] * buf[i + 2];
		    break;

		case 0x21:	// load
		    if (i == 0)
		    {
			if (buf[4] > 0)
			    load = (100 * buf[i + 2]) / buf[i + 1];
			else
			    load = 100;
		    }
		    break;

		case 0x0f:	// injection time
		    inj_time = 0.01 * buf[i + 1] * buf[i + 2];
		    break;

		case 0x12:	// absolute pressure
		    oil_press = 0.04 * buf[i + 1] * buf[i + 2];
		    break;

		case 0x05:	// temperature
		    if (i == 6)
			temp1 = buf[i + 1] * (buf[i + 2] - 100) * 0.1;
		    if (i == 9)
			temp2 = buf[i + 1] * (buf[i + 2] - 100) * 0.1;
		    break;

		case 0x07:	// speed
		    speed = 0.01 * buf[i + 1] * buf[i + 2];
		    break;

		case 0x15:	// battery voltage
		    voltage = 0.001 * buf[i + 1] * buf[i + 2];
		    break;

		default:
#ifdef DEBUG
		    printf ("unknown value: 0x%02x: a = %d, b = %d\n",
			    buf[i], buf[i + 1], buf[i + 2]);
#endif
		    break;
	    }

	}

    }
#ifdef DEBUG
    else
	printf ("\n");
#endif

    /* read block end */
    read (fd, &c, 1);
    if (c != 0x03)
    {
	printf ("block end error (0x03 != 0x%02x)\n", c);
	kw1281_handle_error ();
    }

    kw1281_inc_counter ();

    // set ready flag when receiving ack block
    if (t == 0x09 && !ready)
    {
	ready = 1;
    }
    // set ready flag when sending 0x00 block after errors
    if (t == 0x00 && !ready)
    {
	ready = 1;
    }

}


int
kw1281_setup (char *device)
{
    struct termios oldtio, newtio;
    struct serial_struct st, ot;

    // open the serial device
    fd = open (device, O_SYNC | O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
	printf ("couldn't open serial device %s.\n", device);
	return -1;
    }

    tcgetattr (fd, &oldtio);
    if (ioctl (fd, TIOCGSERIAL, &ot) < 0)
    {
	printf ("getting tio failed\n");
	return -1;
    }
    memcpy (&st, &ot, sizeof (ot));

    // setting custom baud rate
    st.custom_divisor = st.baud_base / 10400;
    st.flags &= ~ASYNC_SPD_MASK;
    st.flags |= ASYNC_SPD_CUST | ASYNC_LOW_LATENCY;
    if (ioctl (fd, TIOCSSERIAL, &st) < 0)
    {
	printf ("TIOCSSERIAL failed\n");
	return -1;
    }

    newtio.c_cflag = B38400 | CLOCAL | CREAD;	// 38400 baud, so custom baud rate above works
    newtio.c_iflag = IGNPAR;	// ICRNL provokes bogus replys after block 12
    newtio.c_oflag = 0;
    newtio.c_cc[VMIN] = 1;
    newtio.c_cc[VTIME] = 0;
    tcflush (fd, TCIFLUSH);
    tcsetattr (fd, TCSANOW, &newtio);

    /* tcsetattr (fd, TCSANOW, &oldtio); */
}

/* this function prints the collected values */
void
output_values (void)
{
    printf ("----------------------------------------\n");
    printf ("l/h\t\t%.2f\n", l_per_h);
    printf ("l/100km\t\t%.2f\n", l_per_100km);
    printf ("speed\t\t%.1f km/h\n", speed);
    printf ("rpm\t\t%.0f RPM\n", rpm);
    printf ("inj on time\t%.2f ms\n", inj_time);
    printf ("temp1\t\t%.1f °C\n", temp1);
    printf ("temp2\t\t%.1f °C\n", temp2);
    printf ("voltage\t\t%.2f V\n", voltage);
    printf ("load\t\t%.0f %%\n", load);
    printf ("absolute press\t%.0f mbar\n", oil_press);
    printf ("counter\t\t%d\n", counter);
    printf ("\n");

    return;
}

void
kw1281_mainloop (void)
{
#ifdef DEBUG
    printf ("receive blocks\n");
#endif

    while (!ready)
    {
	kw1281_recv_block (0x00);
	if (!ready)
	    kw1281_send_ack ();
    }

    printf ("init done.\n");
    for (;;)
    {
	// request block 0x02
	kw1281_send_block (0x02);
	kw1281_recv_block (0x02);	// inj_time, rpm, load, oil_press

	// calculate consumption per hour
	if (inj_time > const_inj_subtract)
	    l_per_h = 60 * 4 * const_multiplier *
		rpm * (inj_time - const_inj_subtract);
	else
	    l_per_h = 0;

	rrdtool_update ("rpm.rrd", rpm);
	rrdtool_update ("con_h.rrd", l_per_h);

	// request block 0x05
	kw1281_send_block (0x05);
	kw1281_recv_block (0x05);	// in this block is speed

	// calculate consumption per hour
	if (speed > 0)
	    l_per_100km = (l_per_h / speed) * 100;
	else
	    l_per_100km = -1;

	// update rrdtool databases
	rrdtool_update ("speed.rrd", speed);
	rrdtool_update ("con_km.rrd", l_per_100km);

	// request block 0x04
	kw1281_send_block (0x04);
	kw1281_recv_block (0x04);	// temperatures + voltage

	// update rrdtool databases
	rrdtool_update ("temp1.rrd", temp1);
	rrdtool_update ("temp2.rrd", temp2);
	rrdtool_update ("voltage.rrd", voltage);

	// output values
	output_values ();
    }
}

int
main (int arc, char **argv)
{
    pid_t   pid1;
    pid_t   pid2;

    if (kw1281_setup ("/dev/ttyUSB0") == -1)
	return -1;

    if ((pid1 = fork ()) == 0)
    {
	ajax_socket (80);
	exit (0);
    }

    /* 
     * another fork for generating rrdtool images
     */

    printf ("init\n");		// ECU: 0x01, INSTR: 0x17
    kw1281_init (0x01);		// send 5baud address, read sync byte + key word

    kw1281_mainloop ();

    return 0;
}


int
ajax_socket (int port)
{
    int     cli, srv;
    int     clisize;

    struct sockaddr_in cliaddr;


    srv = tcp_listen (port);

    for (;;)
    {
	clisize = sizeof (cliaddr);
	if ((cli = accept (srv, &cliaddr, &clisize)) == -1)
	    continue;

	if (fork () == 0)
	{
	    close (srv);
	    handle_client (cli);
	    exit (0);
	}

	close (cli);
    }

    return 0;
}

int
tcp_listen (int port)
{

    int     on = 1;
    int     sock;

    struct sockaddr_in servaddr;

    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
	perror ("socket() failed");
	exit (-1);
    }

    setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));

    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons (port);

    if (bind (sock, &servaddr, sizeof (servaddr)) == -1)
    {
	perror ("bind() failed");
	exit (-1);
    }

    if (listen (sock, 3) == -1)
    {
	perror ("listen() failed");
	exit (-1);
    }

    return sock;
}

int
handle_client (int connfd)
{
    char    recv_buf[1024];
    char    send_buf[1024];

    FILE   *fd;
    char   *line;


    if (readline (connfd, recv_buf, sizeof (recv_buf)) <= 0)
	return 0;

    cut_crlf (recv_buf);

    if (!strcmp (recv_buf, "GET / HTTP/1.1"))
    {
	printf ("GET ");
	// read and ignore all http headers
	while (strcmp (recv_buf, ""))
	{
	    if (readline (connfd, recv_buf, sizeof (recv_buf)) <= 0)
		return 0;

	    cut_crlf (recv_buf);
	}

	// send the html file
	fd = open_html ("ajax.html");

	printf ("ajax.html\n");
	while ((line = get_line (fd)) != NULL)
	{
	    send (connfd, line, strlen (line), 0);
	}
    }

    else if (!strcmp (recv_buf, "POST /update_consumption HTTP/1.1"))
    {
	//printf("POST ");

	// read and ignore headers
	while (strcmp (recv_buf, ""))
	{
	    if (readline (connfd, recv_buf, sizeof (recv_buf)) <= 0)
		return 0;

	    cut_crlf (recv_buf);
	}

	char    buf[256];
	snprintf (buf, sizeof (buf), "%.02f", l_per_100km);
	//printf("%s\n", buf);
	send (connfd, buf, strlen (buf), 0);
    }
    else if (!strcmp (recv_buf, "POST /update_speed HTTP/1.1"))
    {
	//printf("POST ");

	// read and ignore headers
	while (strcmp (recv_buf, ""))
	{
	    if (readline (connfd, recv_buf, sizeof (recv_buf)) <= 0)
		return 0;

	    cut_crlf (recv_buf);
	}

	char    buf[256];
	snprintf (buf, sizeof (buf), "%.01f", speed);
	//printf("%s\n", buf);
	send (connfd, buf, strlen (buf), 0);
    }
    else
    {
	printf ("got something else (%s)\n", recv_buf);
    }

    return 0;
}

void
cut_crlf (char *stuff)
{

    char   *p;

    p = strchr (stuff, '\r');
    if (p)
	*p = '\0';

    p = strchr (stuff, '\n');
    if (p)
	*p = '\0';
}

ssize_t
readline (int fd, void *vptr, size_t maxlen)
{

    ssize_t n, rc;
    char    c, *ptr;

    ptr = vptr;

    for (n = 1; n < maxlen; n++)
    {

      again:
	if ((rc = read (fd, &c, 1)) == 1)
	{
	    *ptr++ = c;
	    if (c == '\n')
		break;
	}
	else if (rc == 0)
	{
	    if (n == 1)
		return 0;
	    else
		break;
	}
	else
	{
	    if (errno == EINTR)
		goto again;
	    return -1;
	}
    }

    *ptr = 0;
    return n;
}


char   *
get_line (FILE * fz)
{
    char    buffy[1024];
    char   *p;

    if (fgets (buffy, 1024, fz) == NULL)
	return NULL;

    // cut_crlf(buffy);
    p = buffy;

    return p;
}



FILE   *
open_html (char *file)
{

    FILE   *fd;

    if ((fd = fopen (file, "r")) == NULL)
    {
	perror ("could not read html file");
	exit (-1);
    }

    return fd;
}
