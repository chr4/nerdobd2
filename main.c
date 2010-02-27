#include "serial.h"

int
main (int arc, char **argv)
{
	pthread_t thread1, thread2;
	
    if (kw1281_open("/dev/ttyUSB0") == -1)
		return -1;

	pthread_create( &thread1, NULL, ajax_socket, 80);

    /* 
     * another fork for generating rrdtool images
     */

    printf ("init\n");		// ECU: 0x01, INSTR: 0x17
    kw1281_init (0x01);		// send 5baud address, read sync byte + key word

	pthread_create( &thread2, NULL, kw1281_mainloop, NULL);

	// wait for thread2
	pthread_join( thread2, NULL); 
	
    return 0;
}
