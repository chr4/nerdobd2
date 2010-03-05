#include "serial.h"

/*
 * start like this:
 * $ while (true); do ./serial; done
 *
 * TODO:
 * 
 * create nice looking interface
 * 
 *
 */

int
main (int arc, char **argv)
{
    pthread_t thread1;
    int fd;

#ifdef SERIAL_ATTACHED
    // kw1281_open() somehow has to be started
    // before any threading stuff.
    if (kw1281_open (DEVICE) == -1)
       return -1;
#endif

    // create databases, unless they exist
    rrdtool_create_consumption ();
    rrdtool_create_speed ();

    /* init values with -2, so ajax socket can control if data is availiable */
    speed = -2;
    rpm = -2;
    temp1 = -2;
    temp2 = -2;
    oil_press = -2;
    inj_time = -2;
    load = -2;
    voltage = -2;
    con_h = -2;
    con_km = -2;
    con_av = -2;
    
    // init average consumption counter
    con_av_counter = 0;
    con_av_array_full = 0;
    memset(&con_av_array, '0', sizeof(con_av_array));
    
    // read con_av_counter and con_av_array from file
    if ( (fd = open( "con_av.dat", O_RDONLY )) != -1)
    {
        read(fd, &con_av_counter, sizeof(con_av_counter));
        read(fd, &con_av_array, sizeof(con_av_array));       
        read(fd, &con_av_array_full, sizeof(con_av_array_full));
        close( fd );
        
#ifdef DEBUG  
        int i;
        printf("con_av_counter: %d\n", con_av_counter);
        for (i = 0; i < con_av_counter; i++)
            printf("%f ", con_av_array[i]);
        printf("array full? (%d)\n", con_av_array_full);
#endif
        
    }

    
    // create ajax socket in new thread for handling http connections
    pthread_create (&thread1, NULL, ajax_socket, (void *) PORT);

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

        if (kw1281_mainloop() == -1)
        {
            printf("errors. exiting...\n");
            ajax_shutdown();
            pthread_kill(thread1, SIGTERM);
            pthread_join(thread1, NULL);
            return -1;
        }
        
    }
    
    return 0;
}
