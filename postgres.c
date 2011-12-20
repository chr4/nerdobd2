#include "postgres.h"


int
exec_query(PGconn *db, char *query)
{
    PGresult   *res;

    res = PQexec(db, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "query '%s' failed: %s", query, PQerrorMessage(db));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    return 0;
}


PGconn *
open_db(void)
{
    PGconn *db;
   
    db = PQconnectdb(DB_POSTGRES);
    if (PQstatus(db) != CONNECTION_OK)
    {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(db));
        close_db(db);
        return NULL;
    }
    
    return db;
}
        

void
init_db(PGconn *db)
{
    // create data table
    exec_query(db, "CREATE TABLE IF NOT EXISTS data ( \
                        id                    SERIAL, \
                        time                  DATE, \
                        rpm                   FLOAT, \
                        speed                 FLOAT, \
                        injection_time        FLOAT, \
                        oil_pressure          FLOAT, \
                        consumption_per_100km FLOAT, \
                        consumption_per_h     FLOAT, \
                        duration_consumption  FLOAT, \
                        duration_speed        FLOAT, \
                        liters                FLOAT, \
                        kilometers            FLOAT, \
                        temp_engine           FLOAT, \
                        temp_air_intake       FLOAT, \
                        voltage               FLOAT, \
                        gps_mode              INTEGER, \
                        gps_latitude          FLOAT, \
                        gps_longitude         FLOAT, \
                        gps_altitude          FLOAT, \
                        gps_speed             FLOAT, \
                        gps_climb             FLOAT, \
                        gps_track             FLOAT, \
                        gps_err_latitude      FLOAT, \
                        gps_err_longitude     FLOAT, \
                        gps_err_altitude      FLOAT, \
                        gps_err_speed         FLOAT, \
                        gps_err_climb         FLOAT, \
                        gps_err_track         FLOAT )");


    // create table where set point information is stored
    exec_query(db, "CREATE TABLE IF NOT EXISTS setpoints ( \
                        id          SERIAL, \
                        name        VARCHAR, \
                        time        DATE, \
                        data        INTEGER)");


    // since postgres doesn't support replace into, we're defining our own set_setpoints function
    exec_query(db, "CREATE OR REPLACE FUNCTION set_setpoint(text) RETURNS void AS $$ \
               BEGIN \
                   IF EXISTS( SELECT name FROM setpoints WHERE name = $1 ) THEN \
                       UPDATE setpoints SET time = current_timestamp, data = ( \
                           SELECT CASE WHEN count(*) = 0 \
                           THEN 0 \
                           ELSE id END \
                           FROM data \
                           GROUP BY id \
                           ORDER BY id DESC LIMIT 1 ) \
                       WHERE name = 'startup'; \
                   ELSE \
                       INSERT INTO setpoints VALUES( DEFAULT, $1, current_timestamp, ( \
                           SELECT CASE WHEN count(*) = 0 \
                           THEN 0 \
                           ELSE id END \
                           FROM data \
                           GROUP BY data.id \
                           ORDER BY data.id DESC LIMIT 1 ) ); \
                   END IF; \
                   RETURN; \
               END; \
               $$ LANGUAGE plpgsql;");
    
    return;
}



void
close_db(PGconn *db)
{
    PQfinish(db);
}
