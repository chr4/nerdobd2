#include "core.h"
#include "../common.h"

int
db_connect(void)
{
    int    db;
    struct sockaddr_un address;
    size_t address_length;

    // open connection to sql server 
    if ( (db = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() failed");
        return -1;
    }

    address.sun_family = AF_UNIX;
    address_length = sizeof(address.sun_family) +
                     sprintf(address.sun_path, UNIX_SOCKET);

    if ( connect(db, (struct sockaddr *) &address, address_length) != 0)
    {
        perror("connect() failed");
        return -1;
    }

    return db;
}


int
db_send(char *name, float value)
{
   int  db;
   int  n;
   char data[256];

   /* we have to connect and close the connection
    * each time, because otherwise the serial connection
    * will become hickups (SIGPIPE)
    */
   if ( (db = db_connect()) == -1)
       return -1;

   n = snprintf(data, 256, "%s:%f", name, value);

   if (write (db, data, n) <= 0)
   {
       printf("db_send: write() error");
       return -1;
   }

   close(db);
   return 0;
}
