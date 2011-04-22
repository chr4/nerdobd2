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

    // we cannot use json library, so we have to do json by hand
    snprintf(query, sizeof(query),
             "{ \"rpm\": %f, \"speed\": %f, \"injection_time\": %f, \
                \"oil_pressure\": %f, \"con_per_100km\": %f, \
                \"con_per_h\": %f \
              }",
              engine.rpm, engine.speed, engine.injection_time,
              engine.oil_pressure, engine.per_km, engine.per_h);

    db_send_query(query);
}

void
db_send_other_data(other_data other)
{
    char query[LEN_QUERY];

    snprintf(query, sizeof(query), 
             "{ \"temp_engine\": %f, \
                \"temp_air_intake\": %f, \
                \"voltage\": %f }", 
             other.temp_engine, other.temp_air_intake, other.voltage);

    db_send_query(query);
}
