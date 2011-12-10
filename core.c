#include "core.h"

// sqlite database handle
sqlite3 *db;


void	cleanup (int);
char	cleaning_up = 0;

pid_t   pid_httpd = -1;

void
wait4childs(void)
{
    do {
        wait(NULL);
    } while (errno != ECHILD);

    return;
}


void
cleanup (int signo)
{
    // if we're already cleaning up, do nothing
    if (cleaning_up)
        return;
	
    cleaning_up = 1;

    // shutdown httpd
    printf("sending SIGTERM to httpd (%d)\n", pid_httpd);
    if (pid_httpd != -1)
        kill(pid_httpd, SIGTERM);

    // close database and sync file to disk
    printf("closing database...\n");
    close_db(db);

    // close serial port
    printf("shutting down serial port...\n");
    kw1281_close();

    // wait for all child processes
    printf("waiting for child processes to finish...\n");
    wait4childs();

    // sync database to disk (without nicing)
    sync2disk(0);
    wait4childs();

    printf("exiting\n");    
    exit(0);
}


void
sig_chld(int signo)
{
        pid_t pid;
        int   stat;

        while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0);
        return;
}


int
main (int argc, char **argv)
{
    int     ret;

    // catch orphans
    signal(SIGCHLD, sig_chld);

    // sync database from disk to ram
    if (sync2ram() == -1)
        return -1;

    // open it
    if ( (db = open_db()) == NULL)
        return -1;

    // and initialize
    init_db(db);

    // start http server
    puts("fireing up httpd server");
    if ( (pid_httpd = httpd_start(db)) == -1)
        cleanup(15);

    // collect gps data
    puts("connecting to gpsd, collecting gps data");
    if (gps_start() == -1)
        cleanup(15); 

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
    
    for (; ;)
    {
        /* disabled, httpd crashed sometimes
         * maybe because db is inconsistent

        // sync database file to disk, using low priority
        sync2disk(19);
        */

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

            // set the first run flags
            consumption_first_run = 1;
            speed_first_run = 1;
        }
        
        printf ("init\n");

        // ECU: 0x01, INSTR: 0x17
        // send 5baud address, read sync byte + key word
        ret = kw1281_init(0x01, ret);

        // soft error, e.g. communication error
        if (ret == SERIAL_SOFT_ERROR)
        {
            printf("init failed, retrying...\n");
            continue;
        }

        // hard error (e.g. serial cable unplugged)
        else if (ret == SERIAL_HARD_ERROR)
        {
            printf("serial port error, trying to recover...\n");
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
add_value(char *s, double value)
{
    char buffer[LEN_QUERY];

    strncpy(buffer, s, LEN_QUERY);

    if (!isnan(value))
        snprintf(s, LEN_QUERY, "%s, %f", buffer, value);
    else
        snprintf(s, LEN_QUERY, "%s, ''", buffer);
}

void
insert_engine_data(engine_data e)
{
    char query[LEN_QUERY];
    static struct gps_fix_t g; 

    exec_query(db, "BEGIN TRANSACTION");

    strlcpy(query, 
            "INSERT INTO engine_data VALUES ( NULL, DATETIME('NOW', 'localtime')",
            sizeof(query) );

    // add engine data
    add_value(query, e.rpm);
    add_value(query, e.speed);
    add_value(query, e.injection_time);
    add_value(query, e.oil_pressure);
    add_value(query, e.consumption_per_100km);
    add_value(query, e.consumption_per_h);
    add_value(query, e.duration_consumption);
    add_value(query, e.duration_speed);
    add_value(query, e.consumption_per_h / 3600 * e.duration_consumption);
    add_value(query, e.speed / 3600 * e.duration_speed);

    // add gps data, if available
    if ( get_gps_data(&g) == 0)
    {
        add_value(query, (double) g.mode);
        add_value(query, g.latitude);
        add_value(query, g.longitude);
        add_value(query, g.altitude);
        add_value(query, g.speed);
        add_value(query, g.climb);
        add_value(query, g.track);
        add_value(query, g.epy);
        add_value(query, g.epx);
        add_value(query, g.epv);
        add_value(query, g.eps);
        add_value(query, g.epc);
        add_value(query, g.epd);
    }
    else
    {
        puts("couldn't get gps data");
        // fill in empty column fields for gps data
        strlcat(query,
                ", '', '', '', '', '', '', '', '', '', '', '', '', ''",
                sizeof(query));
    }

    strlcat(query, " )\n", sizeof(query));

    exec_query(db, query);

    exec_query(db, "END TRANSACTION");
}

void
insert_other_data(other_data o)
{
    char query[LEN_QUERY];

    exec_query(db, "BEGIN TRANSACTION");

    strlcpy(query,
            "INSERT INTO other_data VALUES ( NULL, DATETIME('NOW', 'localtime')",
            sizeof(query) );

    // add other data
    add_value(query, o.temp_engine);
    add_value(query, o.temp_air_intake);
    add_value(query, o.voltage);

    strlcat(query, " )\n", sizeof(query));

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
        engine.consumption_per_h = MULTIPLIER * engine.rpm * engine.injection_time;

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
