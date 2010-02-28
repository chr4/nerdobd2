#include "serial.h"

int
main (int arc, char **argv)
{
    pthread_t thread1;

#ifdef SERIAL_ATTACHED
    // kw1281_open() somehow has to be started
    // before any threading stuff.
    if (kw1281_open (DEVICE) == -1)
       return -1;
#endif

    // create databases, unless they exist
    rrdtool_create_consumption ();
    rrdtool_create_speed ();

    // create ajax socket in new thread for handling http connections
    pthread_create (&thread1, NULL, ajax_socket, (void *) 80);

    for ( ; ; )
    {
#ifdef SERIAL_ATTACHED
        printf ("init\n");                
        
        // ECU: 0x01, INSTR: 0x17
        // send 5baud address, read sync byte + key word
        if (kw1281_init (0x01) == -1)
        {
            printf("init failed. trying again...\n");
            continue;
        }
#endif

        kw1281_mainloop();
    }
    
    return 0;
}