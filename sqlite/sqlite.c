#include "sqlite.h"
#include "../common/config.h"

// the database handler
sqlite3 *db;

int use_hd_db(void);

int create_table(char *);
int exec_query(char *);
int create_table(char *);
int init_db(void);
void close_db(void);
void sync_db(void);

int 
exec_query(char *query)
{

    sqlite3_stmt  *stmt;
    
    int ret;
#ifdef DEBUG_SQLITE
    printf("sql: %s\n", query);
#endif

    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) != SQLITE_OK)
    {
        printf("sqlite3_prepare_v2() error\n");
        return -1;
    }

    ret = sqlite3_step(stmt);

    // database is busy, retry query
    if (ret == SQLITE_BUSY)
    {
        // wait for 0.5 sec
        usleep(500000);

#ifdef DEBUG_SQLITE
        printf("retrying query...\n");

        return exec_query(query);
        printf("SUCCESSFULLY RETRIED!\n");

        return 0;
#else
        return exec_query(query);
#endif

    }

    if (sqlite3_finalize(stmt) != SQLITE_OK)
    {
        printf("sqlite3_finalize() error\n");
    }

    return 0;
}


int
create_table(char *name)
{
    char query[LEN_QUERY];

    snprintf(query, sizeof(query), 
        "CREATE TABLE IF NOT EXISTS %s ( \
             id    INTEGER PRIMARY KEY, \
             time  DATE, \
             value FLOAT \
         )", name);

    return exec_query(query);
}


int
init_db(void)
{

    /* check if database file exists in /dev/shm
     * if not, look for one on the disk and use it
     * if nothing is found, create a new one.
     */
    if (access(DB_RAM, W_OK) == 0)
    {
#ifdef DEBUG_SQLITE
        printf("%s exists and is writeable\n", DB_RAM);
#endif
    }
    else if (access(DB_DISK, R_OK) == 0)
    {
#ifdef DEBUG_SQLITE
        printf("%s exists and is readable\n", DB_DISK);
#endif
        use_hd_db();
    }
    else
    {
#ifdef DEBUG_SQLITE
        printf("no db file found\n");
#endif
    }

    // open database file
    if (sqlite3_open(DB_RAM, &db))
    {
        printf("Can not open database: %s", DB_RAM);
        return -1;
    }
  
    // create tables (if not existent) 
    create_table("temp_engine");
    create_table("temp_air_intake");
    create_table("voltage");

    // create engine_data table
    exec_query("CREATE TABLE IF NOT EXISTS engine_data ( \
                    id             INTEGER PRIMARY KEY, \
                    time           DATE, \
                    rpm            FLOAT, \
                    speed          FLOAT, \
                    injection_time FLOAT, \
                    oil_pressure   FLOAT, \
                    per_km         FLOAT, \
                    per_h          FLOAT \
                )");

    return 0;
}

void
close_db(void)
{
    sqlite3_close(db);
}


// save the db to disk once in a while
void
sync_db(void)
{
    int   status;
    pid_t child;

    if ( (child = fork()) > 0)
    {
        printf("syncing db file to disk...\n");
        execlp("rsync", "rsync", "-a", DB_RAM, DB_DISK, NULL);
        _exit(1);
    }
    else
    {
        while(waitpid(child, &status, WNOHANG) > 0);
    }
}


// copy over disk file to ram
int
use_hd_db(void)
{
    int hd;
    int ram;
    int n;
    void *buf;

    buf = malloc(sizeof(void) * LEN);

    if ( (hd = open(DB_DISK, O_RDONLY)) == -1)
    {
        perror("open");
        return -1;
    }

    if ( (ram = open(DB_RAM, O_CREAT | O_RDWR, 0644)) == -1)
    {
        perror("open");
        return -1;
    }

    do
    {
        n = read(hd, buf, sizeof(buf));
        write(ram, buf, n);
    }
    while (n);

    close(hd);
    close(ram);

    return 0;
}


// get averages
json_object *
json_averages(int timespan)
{
    char          query[LEN_QUERY];
    sqlite3_stmt  *stmt;
    int           ret;

    json_object *averages = json_object_new_object();

    /* averages from current timespan displayed
       averages since last manual reset
       averages since beginning of calculation
     */
    snprintf(query, sizeof(query),
             "SELECT SUM(speed*per_km)/SUM(speed), AVG(per_km) FROM engine_data \
              WHERE time > DATETIME('NOW', '-%d minutes')",
              timespan);

    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) != SQLITE_OK)
    {
        printf("couldn't execute query: '%s'\n", query);
        return NULL;
    }

    do
    {
        ret = sqlite3_step(stmt);

        // database is busy, retry query
        if (ret == SQLITE_BUSY)
        {
            // wait for 0.5 sec
            usleep(500000);

#ifdef DEBUG_SQLITE
            printf("retrying query: %s\n", query);
#endif
            continue;
        }

        if (ret == SQLITE_ROW)
        {
            add_double(averages, "timespan", sqlite3_column_double(stmt, 1));
            add_double(averages, "new", sqlite3_column_double(stmt, 0));
        }
    } while(ret != SQLITE_DONE);

    sqlite3_finalize(stmt);

    return averages;
}

// get latest engine data from database
json_object *
json_latest_data(void)
{
    char          query[LEN_QUERY];
    sqlite3_stmt  *stmt;
    int           ret;

    json_object *data = json_object_new_object();

    snprintf(query, sizeof(query),
             "SELECT   engine_data.*, \
                       voltage.value, temp_engine.value, temp_air_intake.value \
              FROM     engine_data, voltage, temp_engine, temp_air_intake \
              ORDER BY id \
              DESC LIMIT 1");


    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) != SQLITE_OK)
    {
        printf("couldn't execute query: '%s'\n", query);
        return NULL;
    }

    do
    {
        ret = sqlite3_step(stmt);

        // database is busy, retry query
        if (ret == SQLITE_BUSY)
        {
            // wait for 0.5 sec
            usleep(500000);

#ifdef DEBUG_SQLITE
            printf("retrying query: %s\n", query);
#endif
            continue;
        }

        if (ret == SQLITE_ROW)
        {
            add_double(data, "rpm", sqlite3_column_double(stmt, 2));
            add_double(data, "speed", sqlite3_column_double(stmt, 3));
            add_double(data, "injection_time", sqlite3_column_double(stmt, 4));
            add_double(data, "oil_pressure", sqlite3_column_double(stmt, 5));
            add_double(data, "per_km", sqlite3_column_double(stmt, 6));
            add_double(data, "per_h", sqlite3_column_double(stmt, 7));
            add_double(data, "voltage", sqlite3_column_double(stmt, 8));
            add_double(data, "temp_engine", sqlite3_column_double(stmt, 9));
            add_double(data, "temp_air_intake", sqlite3_column_double(stmt, 10));
        }
    } while(ret != SQLITE_DONE);

    sqlite3_finalize(stmt);

    return data;
}

json_object *
json_generate_graph(char *key, int span)
{
    char          query[LEN_QUERY];
    sqlite3_stmt  *stmt;
    int           ret;
    int           index = 0;

    json_object *graph = json_object_new_object();
    json_object *data = add_array(graph, "data");

    snprintf(query, sizeof(query),
             "SELECT id, %s, strftime('%%s000', time) \
              FROM   engine_data \
              WHERE id > %d \
              ORDER BY time",
             key, span);

#ifdef DEBUG_SQLITE
    printf("sql: %s\n", query);
#endif

    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) != SQLITE_OK)
    {
        printf("couldn't execute query: '%s'\n", query);
        return NULL;
    }

    do
    {
        ret = sqlite3_step(stmt);

        // database is busy, retry query
        if (ret == SQLITE_BUSY)
        {
#ifdef DEBUG_SQLITE
            printf("retrying query: %s.\n", query);
#endif
            // wait for 0.5 sec
            usleep(500000);
            continue;
        }

        if (ret == SQLITE_ROW) {
            add_data(data, sqlite3_column_double(stmt, 2),
                           sqlite3_column_double(stmt, 1));

            index = sqlite3_column_int(stmt, 0);
        }

    } while(ret != SQLITE_DONE);

    sqlite3_finalize(stmt);

    add_int(graph, "index", index);

    return graph;
}

const char *
json_generate(int span_consumption, int span_speed, int timespan)
{
    json_object *json = json_object_new_object();

    // get latest engine data from database
    json_object_object_add(json, "latest_data", json_latest_data());    

    // get averages
    json_object_object_add(json, "averages", json_averages(timespan));

    // graphing data 
    json_object_object_add(json, "graph_consumption", json_generate_graph("per_km", span_consumption));
    json_object_object_add(json, "graph_speed", json_generate_graph("speed", span_speed));

    return json_object_to_json_string(json);
}
