#include "core.h"
#include "../common/config.h"
#include "../json/json.h"

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
db_send_query(char *query)
{
   int  db;

   /* we have to connect and close the connection
    * each time, because otherwise the serial connection
    * will become hickups (SIGPIPE)
    */
   if ( (db = db_connect()) == -1)
       return -1;

   if (write (db, query, strlen(query)) <= 0)
   {
       printf("db_send: write() error");
       return -1;
   }

   close(db);
   return 0;
}

void
db_send_engine_data(engine_data engine)
{
    char query[LEN_QUERY];

/*
    snprintf(query, sizeof(query), "INSERT INTO engine_data VALUES ( \
                                    NULL, DATETIME('NOW'), \
                                    %f, %f, %f, %f, %f, %f )",
                                    engine.rpm, engine.speed, engine.injection_time,
                                    engine.oil_pressure, engine.per_km, engine.per_h);
*/
    db_send_query("{ \"rpm\": 2000, \"speed\": 70.5 }");
}

void
db_send_other_data(char *key, float value)
{
    char query[LEN_QUERY];

    snprintf(query, sizeof(query), 
             "INSERT INTO %s VALUES ( NULL, DATETIME('NOW'), %f )", 
             key, value);

    db_send_query(query);
}
