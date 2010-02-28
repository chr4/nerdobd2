#include "serial.h"

int
main (int arc, char **argv)
{
    pthread_t thread1, thread2;

    // create databases, unless they exist
    rrdtool_create_consumption ();
    rrdtool_create ("speed");

    /*
       rrdtool_create("rpm");
       rrdtool_create("temp1");
       rrdtool_create("temp2");
       rrdtool_create("voltage");   
     */

    // create ajax socket in new thread for handling http connections
    pthread_create (&thread1, NULL, ajax_socket, (void *) 80);

    /* 
     * another fork for generating rrdtool images
     */

    for (;;)
    {
        pthread_create (&thread2, NULL, kw1281_mainloop, NULL);
        // wait for thread2
        pthread_join (thread2, NULL);
        usleep(5000000);
    }

    // shut down ajax http server
    pthread_kill (thread1, SIGTERM);

    printf ("exiting main\n");
    return 0;
}
