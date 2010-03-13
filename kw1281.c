#include "serial.h"

static void _set_bit (int);

int     kw1281_send_byte_ack (unsigned char);
int     kw1281_sendajax_log_ack (void);
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
int     counter;                // kw1281 protocol block counter
char    got_ack = 0;            // flag (true if ECU send ack block, thus ready to receive block requests)

// save old values
struct termios oldtio;
struct serial_struct ot, st;
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
            ajax_log("read: select() got EINTR\n");
        if (res == -1)
            ajax_log("read: select() failed\n");
        
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
                ajax_log("read: select() got EINTR\n");

            else
                ajax_log("read: select() failed\n");
        }

    } while (res == -1 && errno == EINTR);

    if (res > 0)
    {
        if (read (fd, &c, 1) == -1)
        {
            ajax_log("kw1281_read_timeout: read() error\n");
            return -1;
        }
    }
    else if (res == 0)
    {
        ajax_log("kw1281_read_timeout: timeout occured\n");
        return -1;
    }
    else {
        ajax_log("kw1281_read_timeout: unknown error\n");
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
                ajax_log("write: select() got EINTR\n");
            
            else
                ajax_log("write: select() failed\n");
        }
        
    } while (res == -1 && errno == EINTR);
    
    if (res > 0)
    {
        if (write (fd, &c, 1) <= 0)
        {
            ajax_log("kw1281_write_timeout: write() error\n");
            return -1;
        }
    }
    else if (res == 0)
    {
        ajax_log("kw1281_write_timeout: timeout occured\n");
        return -1;
    }
    else {
        ajax_log("kw1281_write_timeout: unknown error\n");
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
        ajax_log("kw1281_recv_byte_ack: read() error\n");
        return -1;
    }
    
    d = 0xff - c;
    usleep (WRITE_DELAY);
    
    if (kw1281_write_timeout(d) == -1)
    {
        ajax_log("kw1281_recv_byte_ack: write() error\n");
        return -1;
    }
    
    if ( (d = kw1281_read_timeout()) == -1)
    {
        ajax_log("kw1281_recv_byte_ack: read() error\n");
        return -1;
    }
    
    if (0xff - c != d)
    {
        printf ("kw1281_recv_byte_ack: echo error recv: 0x%02x (!= 0x%02x)\n",
                d, 0xff - c);
        ajax_log("kw1281_recv_byte_ack: echo error\n");
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
        ajax_log("kw1281_send_byte_ack: write() error\n");
        return -1;
    }
    
    if ( (d = kw1281_read_timeout()) == -1)
    {
        ajax_log("kw1281_send_byte_ack: read() error\n");
        return -1;
    }
    
    if (c != d)
    {
        printf ("kw1281_send_byte_ack: echo error (0x%02x != 0x%02x)\n", c,
                d);
        ajax_log("kw1281_send_byte_ack: echo error\n");
        return -1;
    }

    if ( (d = kw1281_read_timeout()) == -1)
    {
        ajax_log("kw1281_send_byte_ack: read() error\n");
        return -1;
    }
    
    if (0xff - c != d)
    {
        printf ("kw1281_send_byte_ack: ack error (0x%02x != 0x%02x)\n",
                0xff - c, d);
        ajax_log("kw1281_send_byte_ack: ack error\n");
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

#ifdef DEBUG
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
        ajax_log("kw1281_send_ack: write() error\n");
        return -1;
    }
    
    if ( (c = kw1281_read_timeout()) == -1)
    {
        ajax_log("kw1281_send_ack: read() error\n");
        return -1;
    }
    
    if (c != 0x03)
    {
        printf ("echo error (0x03 != 0x%02x)\n", c);
        ajax_log("echo error\n");
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

#ifdef DEBUG
    printf ("send group reading block %d\n", counter);
#endif

    /* block length */
    if (kw1281_send_byte_ack (0x04) == -1)
    {
        ajax_log("kw1281_send_block() error\n");
        return -1;
    }

    // counter
    if (kw1281_send_byte_ack (kw1281_inc_counter ()) == -1)
    {
        ajax_log("kw1281_send_block() error\n");
        return -1;
    }

    /*  type group reading */
    if (kw1281_send_byte_ack (0x29) == -1)
    {
        ajax_log("kw1281_send_block() error\n");
        return -1;
    }

    /* which group block */
    if (kw1281_send_byte_ack (n) == -1)
    {
        ajax_log("kw1281_send_block() error\n");
        return -1;
    }


    /* block end */
    c = 0x03;
    usleep (WRITE_DELAY);
    
    if (kw1281_write_timeout(c) == -1)
    {
        ajax_log("kw1281_send_block: write() error\n");
        return -1;
    }
    
    if ( (c = kw1281_read_timeout()) == -1)
    {
        ajax_log("kw1281_send_block: read() error\n");
        return -1;
    }
    
    if (c != 0x03)
    {
        printf ("echo error (0x03 != 0x%02x)\n", c);
        ajax_log("echo error\n");
        return -1;
    }
    
    return 0;
}

/* receive a complete block */
int
kw1281_recv_block (unsigned char n)
{
    int     i;
    
    // we need int, so we can capture -1 as well
    int c, l, t;
    //unsigned char c, l, t;
    
    unsigned char buf[256];

    /* block length */
    if ( (l = kw1281_recv_byte_ack ()) == -1)
    {
        ajax_log("kw1281_recv_block() error\n");
        return -1;
    }

    if ( (c = kw1281_recv_byte_ack ()) == -1)
    {
        ajax_log("kw1281_recv_block() error\n");
        return -1;
    }

    if (c != counter)
    {
        printf ("counter error (%d != %d)\n", counter, c);
        ajax_log("counter error\n");

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

        return -1;
    }

    if ( (t = kw1281_recv_byte_ack ()) == -1)
    {
        ajax_log("kw1281_recv_block() error\n");
        return -1;
    }

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
        if ( (c = kw1281_recv_byte_ack ()) == -1)
        {
            ajax_log("kw1281_recv_block() error\n");
            return -1;
        }

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
                case 0x01:        // rpm
                    if (i == 0)
                        gval->rpm = 0.2 * buf[i + 1] * buf[i + 2];
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
                    gval->inj_time = 0.01 * buf[i + 1] * buf[i + 2];
                    break;

                case 0x12:        // absolute pressure
                    gval->oil_press = 0.04 * buf[i + 1] * buf[i + 2];
                    break;

                case 0x05:        // temperature
                    if (i == 6)
                        gval->temp1 = buf[i + 1] * (buf[i + 2] - 100) * 0.1;
                    if (i == 9)
                        gval->temp2 = buf[i + 1] * (buf[i + 2] - 100) * 0.1;
                    break;

                case 0x07:        // speed
                    gval->speed = 0.01 * buf[i + 1] * buf[i + 2];
                    break;

                case 0x15:        // battery voltage
                    gval->voltage = 0.001 * buf[i + 1] * buf[i + 2];
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
    if ( (c = kw1281_read_timeout()) == -1)
    {
        ajax_log("kw1281_recv_block: read() error\n");
        return -1;
    }
    if (c != 0x03)
    {
        printf ("block end error (0x03 != 0x%02x)\n", c);
        ajax_log("block end error\n");
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
        ajax_log("kw1281_get_block() error\n");
        return -1;
    }
    
    if (kw1281_recv_block(n) == -1)
    {
        ajax_log("kw1281_get_block() error\n");
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
        ajax_log ("couldn't open serial device\n");
        sleep(1);
        return -1;
    }

    if (ioctl (fd, TIOCGSERIAL, &ot) < 0)
    {
        ajax_log ("getting tio failed\n");
        return -1;
    }
    memcpy (&st, &ot, sizeof (ot));

    // setting custom baud rate
    st.custom_divisor = st.baud_base / BAUDRATE;
    st.flags &= ~ASYNC_SPD_MASK;
    st.flags |= ASYNC_SPD_CUST | ASYNC_LOW_LATENCY;
    if (ioctl (fd, TIOCSSERIAL, &st) < 0)
    {
        ajax_log ("TIOCSSERIAL failed\n");
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
        ajax_log("tcsetattr() failed.\n");
        return -1;
    }

    return 0;
}

// restore old serial configuration and close port
int
kw1281_close(void)
{
    ajax_log("shutting down serial port\n");
    
    if (ioctl (fd, TIOCSSERIAL, &ot) < 0)
    {
        ajax_log ("TIOCSSERIAL failed\n");
        return -1;
    }
    
    // allow buffer to drain, discard input
    // TCSADRAIN for only letting it drain
    if (tcsetattr (fd, TCSAFLUSH, &oldtio) == -1)
    {
        ajax_log("tcsetattr() failed.\n");
        return -1;
    }

    if (ioctl (fd, TIOCMSET, &oldflags) < 0)
    {
        ajax_log("TIOCMSET failed.\n");
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
    
    
#ifdef DEBUG
    printf("emptying buffer...\n");
#endif 
    
    // empty receive buffer
    kw1281_empty_buffer();
    

#ifdef DEBUG    
    printf("waiting idle time...\n");
#endif    
    
    // wait the idle time
    usleep(300000);
    
    
    // prepare to send (clear dtr and rts)
    if (ioctl (fd, TIOCMGET, &flags) < 0)
    {
        ajax_log("TIOCMGET failed.\n");
        return -2;
    }
    
    // save old flags so we can restore them later
    oldflags = flags;
    
    flags &= ~(TIOCM_DTR | TIOCM_RTS);

    if (ioctl (fd, TIOCMSET, &flags) < 0)
    {
        ajax_log("TIOCMSET failed.\n");
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
        ajax_log("TIOCMGET failed.\n");
        return -2;
    }

     flags |= TIOCM_DTR;
    if (ioctl (fd, TIOCMSET, &flags) < 0)
    {
        ajax_log("TIOCMSET failed.\n");
        return -2;
    }
 
    // read bogus values, if any
    if (ioctl (fd, FIONREAD, &in) < 0)
    {
        ajax_log("FIONREAD failed.\n");
        return -1;
    }

#ifdef DEBUG
    printf("found %d chars to ignore\n", in);
#endif
    
    while (in--)
    {
        if ( (c = kw1281_read_timeout()) == -1)
        {
            ajax_log("kw1281_init: read() error\n");
            return -1;
        }
#ifdef DEBUG
        printf ("ignore 0x%02x\n", c);
#endif
    }
    
    if ( (c = kw1281_read_timeout()) == -1)
    {
        ajax_log("kw1281_init: read() error\n");
        return -1;
    }
#ifdef DEBUG
    printf ("read 0x%02x\n", c);
#endif
    
    if ( (c = kw1281_read_timeout()) == -1)
    {
        ajax_log("kw1281_init: read() error\n");
        return -1;
    }
#ifdef DEBUG
    printf ("read 0x%02x\n", c);
#endif
    
    if ( (c = kw1281_recv_byte_ack ()) == -1)
        return -1;
    
#ifdef DEBUG
    printf ("read 0x%02x (and sent ack)\n", c);
#endif
    
    counter = 1;
    
    return 0;
}

int
kw1281_mainloop (void)
{
    int    status;
    int    file;
    
#ifndef SERIAL_ATTACHED
    /* 
     * this block is for testing purposes
     * when the car is too far to test
     * the html interface / ajax server
     */

    sleep(1);
    ajax_log("incrementing values for testing purposes...\n");
    gval->speed = 10;
    gval->con_km = -1;
    gval->rpm = 1000;
    gval->con_h = 1.01;
    gval->temp1 = 20;
    gval->temp2 = 0;
    gval->voltage = 3.00;
    
    for (;;)
    {
        gval->speed++;
        gval->con_km += 0.75;
        gval->con_h += 1.03;
        
        gval->temp1++;
        gval->temp2++;
        gval->voltage += 0.15;
        gval->rpm += 100;

        // update rrdtool databases
        rrdtool_update_consumption();
        rrdtool_update_speed();
        
        
        // save consumption array to file        
        if ( (file = open( CON_AV_FILE, O_WRONLY|O_CREAT, 00644 )) == -1)
            perror("couldn't open file:\n");
        else
        {
            write(file, av_con, sizeof(struct average));
            close(file);
        }
        
        // save av_speed array to file    
        if ( (file = open( SPEED_AV_FILE, O_WRONLY|O_CREAT, 00644 )) == -1)
            perror("couldn't open file:\n");
        else
        {
            write(file, av_speed, sizeof(struct average));
            close(file);
        }
        
        // collect defunct processes from rrdtool
        while(waitpid(-1, &status, WNOHANG) > 0);
        
        sleep(1);
    }
#endif


#ifdef DEBUG
    printf ("receive blocks\n");
#endif

    
    if (kw1281_get_ascii_blocks() == -1)
        return -1;
    
    ajax_log ("init done.\n");
    for ( ; ; )
    {
        // request block 0x02
        // (inj_time, rpm, load, oil_press)
        if (kw1281_get_block(0x02) == -1)
            return -1;

        // request block 0x05
        // (speed)
        if (kw1281_get_block(0x05) == -1)
            return -1;

        // request block 0x04
        // (temperatures + voltage)
        if (kw1281_get_block(0x04) == -1)
            return -1;
        
        
        /* fork so we don't disrupt time critical
         * serial communication
         */
        if (fork() > 0)
        {
            // calculate consumption per hour
            if (gval->inj_time > INJ_SUBTRACT)
                gval->con_h = 60 * 4 * MULTIPLIER *
                              gval->rpm * (gval->inj_time - INJ_SUBTRACT);
            else
                gval->con_h = 0;
            
            /* calculate how much liters we consumed this second
             * and add it to absolute consumption counter
             */
            av_con->liters += gval->con_h / 3600;
            
            
            // calculate consumption per hour
            if (gval->speed > 5)
                // below 5km/h values get very high, which makes the graphs unreadable
                gval->con_km = (gval->con_h / gval->speed) * 100;
            else
                gval->con_km = -1;
            
        
            // output values
            kw1281_print ();
            
            // update rrdtool databases
            rrdtool_update_consumption();
            rrdtool_update_speed();
            

            // save consumption array to file        
            if ( (file = open( CON_AV_FILE, O_WRONLY|O_CREAT, 00644 )) == -1)
                perror("couldn't open file:\n");
            else
            {
                write(file, av_con, sizeof(struct average));
                close(file);
            }

            // save av_speed array to file    
            if ( (file = open( SPEED_AV_FILE, O_WRONLY|O_CREAT, 00644 )) == -1)
                perror("couldn't open file:\n");
            else
            {
                write(file, av_speed, sizeof(struct average));
                close(file);
            }
        }

        // collect defunct processes functions
        while(waitpid(-1, &status, WNOHANG) > 0);
    }
    
    return 0;
}

// this function prints the collected values 
void
kw1281_print (void)
{
    printf ("----------------------------------------\n");
    printf ("l abs.\t\t%.3f\n", av_con->liters);
    printf ("l/h\t\t%.2f\n", gval->con_h);
    printf ("l/100km\t\t%.2f\n", gval->con_km);
    printf ("speed\t\t%.1f km/h\n", gval->speed);
    printf ("rpm\t\t%.0f RPM\n", gval->rpm);
    printf ("inj on time\t%.2f ms\n", gval->inj_time);
    printf ("temp1\t\t%.1f °C\n", gval->temp1);
    printf ("temp2\t\t%.1f °C\n", gval->temp2);
    printf ("voltage\t\t%.2f V\n", gval->voltage);
    //printf ("load\t\t%.0f %%\n", gval->load);
    printf ("absolute press\t%.0f mbar\n", gval->oil_press);
    printf ("counter\t\t%d\n", counter);
    printf ("\n");
    
    return;
}
