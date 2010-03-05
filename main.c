#include "serial.h"

/*
 * start like this:
 * $ while (true); do ./serial; done
 *
 * TODO:
 * 
 * create nice looking interface
 *
 * apply .js colors, etc through .css file
 * so we can have multiple styles with only changing the css
 * (prolly not possible, because colors have to be changed in C files)
 *
 * create graphical overlay for masking rrdtool watermarks
 * 
 * create fixed line at 6.55 liters (rrdtool, consumtion)
 *
 * fix communication errors with ECU
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

    
    // init average consumption struct
    memset(&consumption.array, '0', sizeof(consumption.array));
    consumption.counter = 0;
    consumption.array_full = 0;
    consumption.average_short = -2;
    consumption.average_medium = -2;
    consumption.average_long = -2;
    
    
    // overwrite consumption inits from file, if present
    if ( (fd = open( CON_AV_FILE, O_RDONLY )) != -1)
    {
        read(fd, &consumption, sizeof(struct con_av));
        close( fd );
    }
    
#ifdef DEBUG  
    int i;
    printf("consumption.counter: %d\n", consumption.counter);
    for (i = 0; i < consumption.counter; i++)
       printf("%.02f ", consumption.array[i]);
    printf("array full? (%d)\n", consumption.array_full);
    printf("consumption averages: %.02f, %.02f, %.02f\n",
           consumption.average_short, consumption.average_medium, consumption.average_long);
#endif    

    
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
