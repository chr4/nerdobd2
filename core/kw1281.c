#include "core.h"
#include "../common/config.h"

static void _set_bit (int);

int     kw1281_send_byte_ack (unsigned char);
int     kw1281_sendprintf_ack (void);
int     kw1281_send_block (unsigned char);
int     kw1281_recv_block (unsigned char);
int     kw1281_send_ack (void);
int     kw1281_recv_byte_ack (void);
int     kw1281_inc_counter (void);
int     kw1281_get_ascii_blocks(void);
int     kw1281_get_block (unsigned char);
int     kw1281_empty_buffer(void);
int     kw1281_read_timeout(void);
int     kw1281_write_timeout(unsigned char c);
void    kw1281_print (void);

int     fd;
int     counter;        // kw1281 protocol block counter
char    got_ack = 0;    // flag (true if ECU send ack block, thus ready to receive block requests)
int     loopcount;      // counter for mainloop


// save old values
struct  termios oldtio;
struct  serial_struct ot, st;
int     oldflags;


// read 1024 bytes with 200ms timeout
int
kw1281_empty_buffer(void)
{
    char c[1024];
    
    int res;
    struct timeval timeout;
    fd_set rfds;                // file descriptor set
    
    // set timeout value within input loop
    timeout.tv_usec = 200000;   // milliseconds
    timeout.tv_sec  = 0.2;      // seconds
    
    
    /* doing do-while to catch EINTR
     * it's not absolutely necessary, but i read
     * somewhere that it's better to do it that way...
     */
    do {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        
        res = select(fd + 1, &rfds, NULL, NULL, &timeout);
        
        if (errno == EINTR)
            printf("read: select() got EINTR\n");
        if (res == -1)
            printf("read: select() failed\n");
        
    } while (res == -1 && errno == EINTR);
    
    if (res > 0)
    {
        if (read (fd, &c, sizeof(c)) == -1)
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }
    
    return 0;    
}

int
kw1281_read_timeout(void)
{
    unsigned char c;
    
    int res;
    struct timeval timeout;
    fd_set rfds;            // file descriptor set
    
    // set timeout value within input loop
    timeout.tv_usec = 0;    // milliseconds
    timeout.tv_sec  = 1;    // seconds
    
    
    /* doing do-while to catch EINTR
     * it's not absolutely necessary, but i read
     * somewhere that it's better to do it that way...
     */
    do {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
    
        res = select(fd + 1, &rfds, NULL, NULL, &timeout);
        
        if (res == -1)
        {
            if (errno == EINTR)
                printf("read: select() got EINTR\n");

            else
                printf("read: select() failed\n");
        }

    } while (res == -1 && errno == EINTR);

    if (res > 0)
    {
        if (read (fd, &c, 1) == -1)
        {
            printf("kw1281_read_timeout: read() error\n");
            return -1;
        }
    }
    else if (res == 0)
    {
        printf("kw1281_read_timeout: timeout occured\n");
        return -1;
    }
    else {
        printf("kw1281_read_timeout: unknown error\n");
        return -1;
    }

    return c;
}


int
kw1281_write_timeout(unsigned char c)
{  
    int res;
    struct timeval timeout;
    
    fd_set wfds;    /* file descriptor set */
    
    /* set timeout value within input loop */
    timeout.tv_usec = 0;  /* milliseconds */
    timeout.tv_sec  = 1;  /* seconds */
    
    /* doing do-while to catch EINTR
     * it's not absolutely necessary, but i read
     * somewhere that it's better to do it that way...
     */
    do {
        FD_ZERO(&wfds);
        FD_SET(fd, &wfds);
    
        res = select(fd + 1, NULL, &wfds, NULL, &timeout);
        
        if (res == -1)
        {
            if (errno == EINTR)
                printf("write: select() got EINTR\n");
            
            else
                printf("write: select() failed\n");
        }
        
    } while (res == -1 && errno == EINTR);
    
    if (res > 0)
    {
        if (write (fd, &c, 1) <= 0)
        {
            printf("kw1281_write_timeout: write() error\n");
            return -1;
        }
    }
    else if (res == 0)
    {
        printf("kw1281_write_timeout: timeout occured\n");
        return -1;
    }
    else {
        printf("kw1281_write_timeout: unknown error\n");
        return -1;
    }

    
    return 0;
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
int
kw1281_recv_byte_ack (void)
{
    // we need int, so we can capture -1 as well
    int c, d;
    // unsigned char c, d;

    if ( (c = kw1281_read_timeout()) == -1)
    {
        printf("kw1281_recv_byte_ack: read() error\n");
        return -1;
    }
    
    d = 0xff - c;
    usleep (WRITE_DELAY);
    
    if (kw1281_write_timeout(d) == -1)
    {
        printf("kw1281_recv_byte_ack: write() error\n");
        return -1;
    }
    
    if ( (d = kw1281_read_timeout()) == -1)
    {
        printf("kw1281_recv_byte_ack: read() error\n");
        return -1;
    }
    
    if (0xff - c != d)
    {
        printf ("kw1281_recv_byte_ack: echo error recv: 0x%02x (!= 0x%02x)\n",
                d, 0xff - c);
        printf("kw1281_recv_byte_ack: echo error\n");
        return -1;
    }
    return c;
}

/* send one byte and wait for acknowledgement */
int
kw1281_send_byte_ack (unsigned char c)
{
    // we need int, so we can capture -1 as well
    int d;
    // unsigned char d;

    usleep (WRITE_DELAY);
    
    if (kw1281_write_timeout(c) == -1)
    {
        printf("kw1281_send_byte_ack: write() error\n");
        return -1;
    }
    
    if ( (d = kw1281_read_timeout()) == -1)
    {
        printf("kw1281_send_byte_ack: read() error\n");
        return -1;
    }
    
    if (c != d)
    {
        printf ("kw1281_send_byte_ack: echo error (0x%02x != 0x%02x)\n", c,
                d);
        printf("kw1281_send_byte_ack: echo error\n");
        return -1;
    }

    if ( (d = kw1281_read_timeout()) == -1)
    {
        printf("kw1281_send_byte_ack: read() error\n");
        return -1;
    }
    
    if (0xff - c != d)
    {
        printf ("kw1281_send_byte_ack: ack error (0x%02x != 0x%02x)\n",
                0xff - c, d);
        printf("kw1281_send_byte_ack: ack error\n");
        return -1;
    }
    
    return 0;
}

// send an ACK block
int
kw1281_send_ack (void)
{
    // we need int, so we can capture -1 as well
    int c;
    // unsigned char c;

#ifdef DEBUG_SERIAL
    printf ("send ACK block %d\n", counter);
#endif

    /* block length */
    if (kw1281_send_byte_ack (0x03) == -1)
        return -1;

    if (kw1281_send_byte_ack (kw1281_inc_counter ()) == -1)
        return -1;

    /* ack command */
    if (kw1281_send_byte_ack (0x09) == -1)
        return -1;

    /* block end */
    c = 0x03;
    usleep (WRITE_DELAY);
    
    if (kw1281_write_timeout(c) == -1)
    {
        printf("kw1281_send_ack: write() error\n");
        return -1;
    }
    
    if ( (c = kw1281_read_timeout()) == -1)
    {
        printf("kw1281_send_ack: read() error\n");
        return -1;
    }
    
    if (c != 0x03)
    {
        printf ("echo error (0x03 != 0x%02x)\n", c);
        printf("echo error\n");
        return -1;
    }

    return 0;
}

/* send group reading block */
int
kw1281_send_block (unsigned char n)
{
    // we need int, so we can capture -1 as well
    int c;
    // unsigned char c;

#ifdef DEBUG_SERIAL
    printf ("send group reading block %d\n", counter);
#endif

    /* block length */
    if (kw1281_send_byte_ack (0x04) == -1)
    {
        printf("kw1281_send_block() error\n");
        return -1;
    }

    // counter
    if (kw1281_send_byte_ack (kw1281_inc_counter ()) == -1)
    {
        printf("kw1281_send_block() error\n");
        return -1;
    }

    /*  type group reading */
    if (kw1281_send_byte_ack (0x29) == -1)
    {
        printf("kw1281_send_block() error\n");
        return -1;
    }

    /* which group block */
    if (kw1281_send_byte_ack (n) == -1)
    {
        printf("kw1281_send_block() error\n");
        return -1;
    }


    /* block end */
    c = 0x03;
    usleep (WRITE_DELAY);
    
    if (kw1281_write_timeout(c) == -1)
    {
        printf("kw1281_send_block: write() error\n");
        return -1;
    }
    
    if ( (c = kw1281_read_timeout()) == -1)
    {
        printf("kw1281_send_block: read() error\n");
        return -1;
    }
    
    if (c != 0x03)
    {
        printf ("echo error (0x03 != 0x%02x)\n", c);
        printf("echo error\n");
        return -1;
    }
    
    return 0;
}

/* receive a complete block */
int
kw1281_recv_block (unsigned char n)
{
    int i;
    
    // we need int, so we can capture -1 as well
    int c, l, t;
    //unsigned char c, l, t;
    
    unsigned char buf[256];

    /* block length */
    if ( (l = kw1281_recv_byte_ack ()) == -1)
    {
        printf("kw1281_recv_block() error\n");
        return -1;
    }

    if ( (c = kw1281_recv_byte_ack ()) == -1)
    {
        printf("kw1281_recv_block() error\n");
        return -1;
    }

    if (c != counter)
    {
        printf ("counter error (%d != %d)\n", counter, c);
        printf("counter error\n");

#ifdef DEBUG_SERIAL
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

        return -1;
    }

    if ( (t = kw1281_recv_byte_ack ()) == -1)
    {
        printf("kw1281_recv_block() error\n");
        return -1;
    }

#ifdef DEBUG_SERIAL
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
        if ( (c = kw1281_recv_byte_ack ()) == -1)
        {
            printf("kw1281_recv_block() error\n");
            return -1;
        }

        buf[i++] = c;

#ifdef DEBUG_SERIAL
        printf ("0x%02x ", c);
#endif

    }
    buf[i] = 0;

#ifdef DEBUG_SERIAL
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
                case 0x01:        // rpm
                    if (i == 0)
                        handle_data("rpm", 0.2 * buf[i + 1] * buf[i + 2]);
                    break;

                /* can't calculate load properly, thus leaving it alone
                case 0x21:        // load
                    if (i == 0)
                    {
                        if (buf[i + 1] == 0)
                            load = 100;
                        else
                            load = 100 * buf[i + 2] / buf[i + 1];
                    }
                    break;
                */
                    
                case 0x0f:        // injection time
                    handle_data("injection_time", 0.01 * buf[i + 1] * buf[i + 2]);
                    break;

                case 0x12:        // absolute pressure
                    handle_data("oil_pressure", 0.04 * buf[i + 1] * buf[i + 2]);
                    break;

                case 0x05:        // temperature
                    if (i == 6)
                        handle_data("temp_engine", buf[i + 1] * (buf[i + 2] - 100) * 0.1);
                    if (i == 9)
                        handle_data("temp_air_intake", buf[i + 1] * (buf[i + 2] - 100) * 0.1);
                    break;

                case 0x07:        // speed
                    handle_data("speed", 0.01 * buf[i + 1] * buf[i + 2]);
                    break;

                case 0x15:        // battery voltage
                    handle_data("voltage", 0.001 * buf[i + 1] * buf[i + 2]);
                    break;

                case 0x13:        // tank content
                    handle_data("tank_content", 0.01 * buf[i + 1] * buf[i + 2]);
                    break;
                    
                default:
#ifdef DEBUG_SERIAL
                    printf ("unknown value: 0x%02x: a = %d, b = %d\n",
                            buf[i], buf[i + 1], buf[i + 2]);
#endif
                    break;
            }

        }

    }
#ifdef DEBUG_SERIAL
    else
        printf ("\n");
#endif

    /* read block end */
    if ( (c = kw1281_read_timeout()) == -1)
    {
        printf("kw1281_recv_block: read() error\n");
        return -1;
    }
    if (c != 0x03)
    {
        printf ("block end error (0x03 != 0x%02x)\n", c);
        printf("block end error\n");
        return -1;
    }

    kw1281_inc_counter ();

    // set ready flag when receiving ack block
    if (t == 0x09 && !got_ack)
    {
        got_ack = 1;
    }
    // set ready flag when sending 0x00 block after errors
    if (t == 0x00 && !got_ack)
    {
        got_ack = 1;
    }
    
    return 0;
}

int
kw1281_get_block (unsigned char n)
{
    if (kw1281_send_block(n) == -1)
    {
        printf("kw1281_get_block() error\n");
        return -1;
    }
    
    if (kw1281_recv_block(n) == -1)
    {
        printf("kw1281_get_block() error\n");
        return -1;
    }
    
    return 0;
}

int
kw1281_get_ascii_blocks(void)
{
    got_ack = 0;
    
    while (!got_ack)
    {
        if (kw1281_recv_block (0x00) == -1)
            return -1;
        
        if (!got_ack)
            if (kw1281_send_ack () == -1)
                return -1;
    }

    return 0;
}

int
kw1281_open (char *device)
{
    struct termios newtio;

    // open the serial device
    if ((fd = open (device, O_SYNC | O_RDWR | O_NOCTTY)) < 0)
    {
        /* we cannot use printf() in this function
         * because shm vars will be initialized afterwards..
         */
        printf ("couldn't open serial device\n");
        sleep(1);
        return -1;
    }

    if (ioctl (fd, TIOCGSERIAL, &ot) < 0)
    {
        printf ("getting tio failed\n");
        return -1;
    }
    memcpy (&st, &ot, sizeof (ot));

    // setting custom baud rate
    st.custom_divisor = st.baud_base / BAUDRATE;
    st.flags &= ~ASYNC_SPD_MASK;
    st.flags |= ASYNC_SPD_CUST | ASYNC_LOW_LATENCY;
    if (ioctl (fd, TIOCSSERIAL, &st) < 0)
    {
        printf ("TIOCSSERIAL failed\n");
        return -1;
    }
    
    tcgetattr (fd, &oldtio);
    
    newtio.c_cflag = B38400 | CLOCAL | CREAD;   // 38400 baud, so custom baud rate above works
    newtio.c_iflag = IGNPAR;                    // ICRNL provokes bogus replys after block 12
    newtio.c_oflag = 0;
    newtio.c_cc[VMIN] = 1;
    newtio.c_cc[VTIME] = 0;
    
    tcflush (fd, TCIFLUSH);
    if (tcsetattr (fd, TCSANOW, &newtio) == -1)
    {
        printf ("tcsetattr() failed.\n");
        return -1;
    }

    return 0;
}

// restore old serial configuration and close port
int
kw1281_close(void)
{
    printf("shutting down serial port\n");
    
    if (ioctl (fd, TIOCSSERIAL, &ot) < 0)
    {
        printf ("TIOCSSERIAL failed\n");
        return -1;
    }
    
    // allow buffer to drain, discard input
    // TCSADRAIN for only letting it drain
    if (tcsetattr (fd, TCSAFLUSH, &oldtio) == -1)
    {
        printf("tcsetattr() failed.\n");
        return -1;
    }

    if (ioctl (fd, TIOCMSET, &oldflags) < 0)
    {
        printf("TIOCMSET failed.\n");
        return -1;
    }
    
    // check close
    if (close(fd))
    {
        perror("close");
    }

    return 0;    
}

/* write 7O1 address byte at 5 baud and wait for sync/keyword bytes */
int
kw1281_init (int address)
{
    int     i, p, flags;

    int c; // we need int so we can capture -1 as well
    // unsigned char c;
    int     in;
    
    
#ifdef DEBUG_SERIAL
    printf("emptying buffer...\n");
#endif 
    
    // empty receive buffer
    kw1281_empty_buffer();
    

#ifdef DEBUG_SERIAL    
    printf("waiting idle time...\n");
#endif    
    
    // wait the idle time
    usleep(300000);
    
    
    // prepare to send (clear dtr and rts)
    if (ioctl (fd, TIOCMGET, &flags) < 0)
    {
        printf("TIOCMGET failed.\n");
        return -2;
    }
    
    // save old flags so we can restore them later
    oldflags = flags;
    
    flags &= ~(TIOCM_DTR | TIOCM_RTS);

    if (ioctl (fd, TIOCMSET, &flags) < 0)
    {
        printf("TIOCMSET failed.\n");
        return -2;
    }

    usleep (INIT_DELAY);
    
    _set_bit (0);               // start bit
    usleep (INIT_DELAY);        // 5 baud
    p = 1;
    for (i = 0; i < 7; i++)
    {
        // address bits, lsb first
        int     bit = (address >> i) & 0x1;
        _set_bit (bit);
        p = p ^ bit;
        usleep (INIT_DELAY);
    }
    _set_bit (p);               // odd parity
    usleep (INIT_DELAY);
    _set_bit (1);               // stop bit
    usleep (INIT_DELAY);
    
    // set dtr
    if (ioctl (fd, TIOCMGET, &flags) < 0)
    {
        printf("TIOCMGET failed.\n");
        return -2;
    }

     flags |= TIOCM_DTR;
    if (ioctl (fd, TIOCMSET, &flags) < 0)
    {
        printf("TIOCMSET failed.\n");
        return -2;
    }
 
    // read bogus values, if any
    if (ioctl (fd, FIONREAD, &in) < 0)
    {
        printf("FIONREAD failed.\n");
        return -1;
    }

#ifdef DEBUG_SERIAL
    printf("found %d chars to ignore\n", in);
#endif
    
    while (in--)
    {
        if ( (c = kw1281_read_timeout()) == -1)
        {
            printf("kw1281_init: read() error\n");
            return -1;
        }
#ifdef DEBUG_SERIAL
        printf ("ignore 0x%02x\n", c);
#endif
    }
    
    if ( (c = kw1281_read_timeout()) == -1)
    {
        printf("kw1281_init: read() error\n");
        return -1;
    }
#ifdef DEBUG_SERIAL
    printf ("read 0x%02x\n", c);
#endif
    
    if ( (c = kw1281_read_timeout()) == -1)
    {
        printf("kw1281_init: read() error\n");
        return -1;
    }
#ifdef DEBUG_SERIAL
    printf ("read 0x%02x\n", c);
#endif
    
    if ( (c = kw1281_recv_byte_ack ()) == -1)
        return -1;
    
#ifdef DEBUG_SERIAL
    printf ("read 0x%02x (and sent ack)\n", c);
#endif
    
    counter = 1;
    
    return 0;
}

int
kw1281_mainloop (void)
{
#ifdef DEBUG_SERIAL
    printf ("receive blocks\n");
#endif

    
    if (kw1281_get_ascii_blocks() == -1)
        return -1;
	
    printf("init done.\n");
    for ( loopcount = 0; ;loopcount++)
    {
        // request block 0x02
        // (inj_time, rpm, load, oil_press)
        if (kw1281_get_block(0x02) == -1)
            return -1;
		
        // request block 0x05
        // (speed)
        if (kw1281_get_block(0x05) == -1)
            return -1;
 
        /* don't request temperatures and
         * voltage too often
         */
        if (! (loopcount % 15) )
        {
            // request block 0x04
            // (temperatures + voltage)
            if (kw1281_get_block(0x04) == -1)
                return -1;
        } 
    }
    
    return 0;
}
