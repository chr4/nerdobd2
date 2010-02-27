#include "serial.h"

#define NO_DEV

int
main (int arc, char **argv)
{
	pthread_t thread1, thread2;
	
#ifndef NO_DEV
    if (kw1281_open("/dev/ttyUSB0") == -1)
		return -1;
#endif
	// create databases, unless they exist
	rrdtool_create("rpm");
	rrdtool_create("con_h");
	rrdtool_create("con_km");
	rrdtool_create("speed");
	rrdtool_create("temp1");
	rrdtool_create("temp2");
	rrdtool_create("voltage");	
	
	// create ajax socket in new thread for handling http connections
	pthread_create( &thread1, NULL, ajax_socket, (void *) 80);

    /* 
     * another fork for generating rrdtool images
     */

	for ( ; ; )
	{
#ifndef NO_DEV
		printf ("init\n");		// ECU: 0x01, INSTR: 0x17
		kw1281_init (0x01);		// send 5baud address, read sync byte + key word
#endif
		pthread_create( &thread2, NULL, kw1281_mainloop, NULL);
		// wait for thread2
		pthread_join( thread2, NULL);
	}
	
	// shut down ajax http server
	pthread_kill( thread1, SIGTERM);
	
	printf("exiting main\n");
    return 0;
}