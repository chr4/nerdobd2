#include "serial.h"

int
main (int arc, char **argv)
{
    pthread_t thread1, thread2;

    // create databases, unless they exist
    rrdtool_create_consumption ();
    rrdtool_create_speed ();

    // create ajax socket in new thread for handling http connections
    pthread_create (&thread1, NULL, ajax_socket, (void *) 80);

    /*
    for (;;)
    {
        pthread_create (&thread2, NULL, kw1281_mainloop, NULL);
        
        // wait for mainloop
        pthread_join (thread2, NULL);
        usleep(INIT_DELAY);
    }
     */
    
    kw1281_mainloop();

    return 0;
}
