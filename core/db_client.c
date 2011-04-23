#include "core.h"

int
db_connect(void)
{
    int    s;
    struct sockaddr_in address;

    // open connection to sql server 
    if ( (s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() failed");
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(DB_HOST);
    address.sin_port = htons(DB_PORT);

    if (connect(s, (struct sockaddr *) &address, sizeof(address)) == -1)
    {
        perror("connect() failed");
        return -1;
    }

    return s;
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
    // we cannot use json library, so we have to do json by hand
    snprintf(query, sizeof(query),
             "{ \"rpm\": %f, \"speed\": %f, \"injection_time\": %f, \
                \"oil_pressure\": %f, \"con_per_100km\": %f, \
                \"con_per_h\": %f \
              }",
              engine.rpm, engine.speed, engine.injection_time,
              engine.oil_pressure, engine.per_km, engine.per_h);
*/

    snprintf(query, sizeof(query), "INSERT INTO engine_data VALUES ( \
                                    NULL, DATETIME('NOW'), \
                                    %f, %f, %f, %f, %f, %f )",
                                    engine.rpm, engine.speed, engine.injection_time,
                                    engine.oil_pressure, engine.per_km, engine.per_h);
/*
    snprintf(query, sizeof(query),
             "POST /engine_data?rpm=%f&speed=%f&injection_time=%f&oil_pressure=%f&con_per_100km=%f&con_per_h=%f HTTP/1.1\r\n\r\n",
              engine.rpm, engine.speed, engine.injection_time,
              engine.oil_pressure, engine.per_km, engine.per_h);
*/
    db_send_query(query);
}

void
db_send_other_data(other_data other)
{
    char query[LEN_QUERY];
/*
    snprintf(query, sizeof(query), 
             "{ \"temp_engine\": %f, \
                \"temp_air_intake\": %f, \
                \"voltage\": %f }", 
             other.temp_engine, other.temp_air_intake, other.voltage);
*/

    snprintf(query, sizeof(query), "INSERT INTO other_data VALUES ( \
                                    NULL, DATETIME('NOW'), \
                                    %f, %f, %f)",
                                    other.temp_engine, other.temp_air_intake, other.voltage);

/*
    snprintf(query, sizeof(query), 
             "POST /other_data?temp_engine=%f&temp_air_intake=%f&voltage=%f HTTP/1.1\r\n\r\n",
             other.temp_engine, other.temp_air_intake, other.voltage);
*/
 //   db_send_query(query);
}
