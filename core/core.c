#include "core.h"

// sqlite database handle
sqlite3 *db;


void	cleanup (int);
char	cleaning_up = 0;

void
cleanup (int signo)
{
    // if we're already cleaning up, do nothing
    if (cleaning_up)
        return;
	
    cleaning_up = 1;
	
    // close database and sync file to disk
    close_db(db);
    sync2disk();

    // close serial port
    kw1281_close();
    
    exit(0);
}


int
main (int argc, char **argv)
{
    int     ret;

    // sync database from disk to ram
    if (sync2ram() == -1)
        return -1;
    
    // open it
    if ( (db = open_db()) == NULL)
        return -1;

    // and initialize
    init_db(db);
    
    
    // add signal handler for cleanup function
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);	


#ifdef TEST
    exec_query(db, "INSERT OR REPLACE INTO setpoints VALUES ( \
                   'startup', DATETIME('now', 'localtime'), ( \
                   SELECT CASE WHEN count(*) = 0 \
                   THEN 0 \
                   ELSE id END \
                   FROM engine_data \
                   ORDER BY id DESC LIMIT 1 \
                   ) \
               )");

    // for testing purposes
    int i = 0, flag = 0;
    for (; ;)
    {
        handle_data("rpm", 1000 + i * 100, 0);
        handle_data("injection_time", 0.15 * i, 1);
        handle_data("oil_pressure", i, 0);
        handle_data("temp_engine", 90, 0);
        handle_data("temp_air_intake", 35, 0);
        handle_data("voltage", 0.01 * i, 0);

        if (!i % 15)
            handle_data("speed", 0, 1);
        else
            handle_data("speed", 3 * i, 1);

        usleep(300000);


        if (i > 35)
            flag = 1;
        if (i < 2)
            flag = 0;

        if (flag)
            i--;
        else
            i++;
    }
#endif

    ret = SERIAL_HARD_ERROR; 
    
    for ( ; ; )
    {
        if (ret == SERIAL_HARD_ERROR)
        {
            while (kw1281_open (DEVICE) == -1)
                usleep(500000);
         
            /* since this restarts the serial connection,
             * it is probably due to a new ride was started, since we
             * set the startup set point to the last index we can find
             */
            exec_query(db, "INSERT OR REPLACE INTO setpoints VALUES ( \
                           'startup', DATETIME('now', 'localtime'), ( \
                           SELECT CASE WHEN count(*) = 0 \
                           THEN 0 \
                           ELSE id END \
                           FROM engine_data \
                           ORDER BY id DESC LIMIT 1 \
                           ) \
                       )");
        }
        
        printf ("init\n");

        // ECU: 0x01, INSTR: 0x17
        // send 5baud address, read sync byte + key word
        ret = kw1281_init(0x01);

        // soft error, e.g. communication error
        if (ret == SERIAL_SOFT_ERROR)
        {
            printf("init failed, retrying...\n");
            continue;
        }

        // hard error (e.g. serial cable unplugged)
        else if (ret == SERIAL_HARD_ERROR)
        {
            printf("serial port error\n");
            kw1281_close();
            continue;
        }


        // start main loop, restart on errors
        if (kw1281_mainloop() == SERIAL_SOFT_ERROR)
        {
            printf("errors. restarting...\n");
            continue;
        }

    }

    // should never be reached
    return 0;
}


void
insert_engine_data(engine_data e)
{
    char query[LEN_QUERY];

    exec_query(db, "BEGIN TRANSACTION");

    snprintf(query, sizeof(query),
             "INSERT INTO engine_data VALUES ( \
             NULL, DATETIME('NOW', 'localtime'), \
             %f, %f, %f, %f, %f, %f, %f, %f, %f, %f )",
             e.rpm, e.speed, e.injection_time,
             e.oil_pressure, e.consumption_per_100km,
             e.consumption_per_h,
             e.duration_consumption, e.duration_speed,
             e.consumption_per_h / 3600 * e.duration_consumption,
             e.speed / 3600 * e.duration_speed);

    exec_query(db, query);

    exec_query(db, "END TRANSACTION");
}

void
insert_other_data(other_data o)
{
    char query[LEN_QUERY];

    exec_query(db, "BEGIN TRANSACTION");

    snprintf(query, sizeof(query),
             "INSERT INTO other_data VALUES ( \
             NULL, DATETIME('NOW', 'localtime'), \
             %f, %f, %f)",
             o.temp_engine, o.temp_air_intake, o.voltage);

    exec_query(db, query);

    exec_query(db, "END TRANSACTION");
}

// this struct collects all engine data
// before sending it to database
engine_data engine;
other_data  other;

void
handle_data(char *name, float value, float duration)
{
    /* first block gets rpm, injection time, oil pressure
     * second block gets speed
     * third block gehts voltage and temperatures (not as often requested)
     */

    if (!strcmp(name, "rpm"))
        engine.rpm = value;
    else if (!strcmp(name, "injection_time"))
    {
        engine.injection_time = value;
        engine.duration_consumption = duration;
    }
    else if (!strcmp(name, "oil_pressure"))
        engine.oil_pressure = value;

    // everytime we get speed, calculate consumption
    // end send data to database
    else if (!strcmp(name, "speed"))
    {
        engine.speed = value;

        // calculate consumption per hour
        engine.consumption_per_h = 60 * 4 * MULTIPLIER * engine.rpm * engine.injection_time;

        // calculate consumption per hour
        if ( engine.speed > 0)
            engine.consumption_per_100km = engine.consumption_per_h / engine.speed * 100;
        else
            engine.consumption_per_100km = -1;

        
        engine.duration_speed = duration;

        insert_engine_data(engine);
    }

    // other data
    else if (!strcmp(name, "temp_engine"))
        other.temp_engine = value;
    else if (!strcmp(name, "temp_air_intake"))
        other.temp_air_intake = value;
    else if (!strcmp(name, "voltage"))
    {
        other.voltage = value;
        insert_other_data(other);
    }
}
