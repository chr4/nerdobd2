#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
//#include <sys/ioctl.h>

#include <linux/termios.h>
#include <linux/serial.h>

int fd;
int counter;			/* kw1281 protocol block counter */
int ready = 0;

/* manually set serial lines */

static void _set_bit(int bit)
{
	int flags;

	ioctl(fd, TIOCMGET, &flags);
	if (bit) {
		ioctl(fd, TIOCCBRK, 0);
		flags &= ~TIOCM_RTS;
	} else {
		ioctl(fd, TIOCSBRK, 0);
		flags |= TIOCM_RTS;
	}
	ioctl(fd, TIOCMSET, &flags);
}

/* receive one byte and acknowledge it */

unsigned char kw1281_recv_byte_ack()
{
	unsigned char c, d;

	read(fd, &c, 1);
	d = 0xff - c;
	usleep(10000);
	write(fd, &d, 1);
	read(fd, &d, 1);
	if (0xff - c != d)
		printf
		    ("    kw1281_recv_byte_ack: echo error recv: 0x%02x (!= 0x%02x)\n",
		     d, 0xff - c);

	return c;
}

/* send one byte and wait for acknowledgement */

void kw1281_send_byte_ack(unsigned char c)
{
	unsigned char d;

	usleep(10000);
	write(fd, &c, 1);
	read(fd, &d, 1);
	if (c != d)
		printf("kw1281_send_byte_ack: echo error (0x%02x != 0x%02x)\n",
		       c, d);
	read(fd, &d, 1);
	if (0xff - c != d)
		printf("kw1281_send_byte_ack: ack error (0x%02x != 0x%02x)\n",
		       0xff - c, d);
}

/* write 7O1 address byte at 5 baud and wait for sync/keyword bytes */

void kw1281_init(int address)
{
	int i, p, flags;
	unsigned char c;

	int in;

	/* prepare to send (clear dtr and rts) */
	ioctl(fd, TIOCMGET, &flags);
	flags &= ~(TIOCM_DTR | TIOCM_RTS);
	ioctl(fd, TIOCMSET, &flags);
	usleep(200000);

	_set_bit(0);		/* start bit */
	usleep(200000);		/* 5 baud */
	p = 1;
	for (i = 0; i < 7; i++) {	/* address bits, lsb first */
		int bit = (address >> i) & 0x1;
		_set_bit(bit);
		p = p ^ bit;
		usleep(200000);
	}
	_set_bit(p);		/* odd parity */
	usleep(200000);
	_set_bit(1);		/* stop bit */
	usleep(200000);

	/* set dtr */
	ioctl(fd, TIOCMGET, &flags);
	flags |= TIOCM_DTR;
	ioctl(fd, TIOCMSET, &flags);

	/* read bogus values, if any */
	ioctl(fd, FIONREAD, &in);
	while (in--) {
		read(fd, &c, 1);
		printf("ignore 0x%02x\n", c);
	}

	read(fd, &c, 1);
	printf("read 0x%02x\n", c);

	read(fd, &c, 1);
	printf("read 0x%02x\n", c);

	c = kw1281_recv_byte_ack();
	prinf("read 0x%02x (and sent ack)\n");

	counter = 1;
}

/* send an ACK block */

void kw1281_send_ack()
{
	unsigned char c;

	printf("send ACK block %d\n", counter);

	/* block length */
	kw1281_send_byte_ack(0x03);

	kw1281_send_byte_ack(counter++);

	/* ack command */
	kw1281_send_byte_ack(0x09);

	/* block end */
	c = 0x03;
	usleep(10000);
	write(fd, &c, 1);
	read(fd, &c, 1);
	if (c != 0x03)
		printf("echo error (0x03 != 0x%02x)\n", c);
}

void kw1281_send_block(unsigned char n)
{
	unsigned char c;

	printf("send group reading block %d\n", counter);

	/* block length */
	kw1281_send_byte_ack(0x04);
	/* counter */
	kw1281_send_byte_ack(counter++);
	/*  type group reading */
	kw1281_send_byte_ack(0x29);
	/* which group block */
	kw1281_send_byte_ack(n);

	/* block end */
	c = 0x03;
	usleep(10000);
	write(fd, &c, 1);
	read(fd, &c, 1);
	if (c != 0x03)
		printf("echo error (0x03 != 0x%02x)\n", c);
}

/* receive a complete block */

void kw1281_recv_block()
{
	int i;
	unsigned char c, l, t;
	unsigned char buf[256];

	/* block length */
	l = kw1281_recv_byte_ack();

	c = kw1281_recv_byte_ack();
	if (c != counter) {
		printf("counter error (%d != %d)\n", counter, c);

		printf("IN   OUT\t(block dump)\n");
		printf("0x%02x\t\t(block length)\n", l);
		printf("     0x%02x\t(ack)\n", 0xff - l);
		printf("0x%02x\t\t(counter)\n", c);
		printf("     0x%02x\t(ack)\n", 0xff - c);

		while (1) {
			c = kw1281_recv_byte_ack();
			printf("0x%02x\t\t(data)\n", c);
			printf("     0x%02x\t(ack)\n", 0xff - c);
		}

	}

	t = kw1281_recv_byte_ack();
	switch (t) {
	case 0xf6:
		printf("got ASCII block %d\n", counter);
		break;
	case 0x09:
		printf("got ACK block %d\n", counter);
		break;
	case 0xe7:
		printf("got group reading answer block %d\n", counter);
		break;
	default:
		printf("block title: 0x%02x (block %d)\n", t, counter);
	}

	l -= 2;

	i = 0;
	while (--l) {
		c = kw1281_recv_byte_ack();
		buf[i++] = c;
		printf("0x%02x ", c);
	}
	buf[i] = 0;
	if (t == 0xf6)
		printf("= \"%s\"\n", buf);
	if (t == 0xe7) {
		printf("\nrpm: %d*%d*0.2\n", buf[1], buf[2]);
		printf("load: 100*%d/%d\n", buf[5], buf[4]);
		printf("inj: %d*%d*0.01\n", buf[7], buf[8]);
		printf("oil: %d*%d*0.04\n", buf[11], buf[10]);
	} else
		printf("\n");

	/* read block end */
	read(fd, &c, 1);
	if (c != 0x03)
		printf("block end error (0x03 != 0x%02x)\n", c);

	counter++;

	if (t == 0x09 && !ready) {
		ready = 1;
	}
}

int main(int arc, char **argv)
{
	unsigned char c, d;
	struct termios oldtio, newtio;
	struct serial_struct st, ot;

	fd = open("/dev/ttyUSB0", O_SYNC | O_RDWR | O_NOCTTY);
	if (fd < 0) {
		printf("couldn't open device..\n");
		exit(-1);
	}

	tcgetattr(fd, &oldtio);

	if (ioctl(fd, TIOCGSERIAL, &ot) < 0) {
		printf("getting tio failed\n");
		return (-1);
	}
	memcpy(&st, &ot, sizeof(ot));

	st.custom_divisor = st.baud_base / 10400;
	st.flags &= ~ASYNC_SPD_MASK;
	st.flags |= ASYNC_SPD_CUST | ASYNC_LOW_LATENCY;
	if (ioctl(fd, TIOCSSERIAL, &st) < 0) {
		printf("TIOCSSERIAL failed\n");
		exit(-1);
	}

	//newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
	//newtio.c_cflag = B4800 | CS8 | CLOCAL | CREAD;     
	//newtio.c_cflag = B19200 | CS8 | CLOCAL | CREAD;
	newtio.c_cflag = B38400 | CLOCAL | CREAD;

	newtio.c_iflag = IGNPAR | ICRNL;
	newtio.c_oflag = 0;
	newtio.c_cc[VMIN] = 1;
	newtio.c_cc[VTIME] = 0;
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);

	printf("init\n");	/* ECU: 0x01, INSTR: 0x17 */
	kw1281_init(0x01);	/* send 5baud address, read sync byte + key word */

	printf("receive blocks\n");
	while (!ready) {
		kw1281_recv_block();
		if (!ready)
			kw1281_send_ack();
	}

	printf("\n\ninit done.\n");
	while (1) {
		kw1281_send_block(0x02);
		kw1281_recv_block();
		kw1281_send_block(0x04);
		kw1281_recv_block();
		kw1281_send_block(0x05);
		kw1281_recv_block();
		usleep (1000000);
	}

	/* tcsetattr (fd, TCSANOW, &oldtio); */
	return 0;
}
