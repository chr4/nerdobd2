#include "sqlite.h"
#include "../common/config.h"

// the database handler
sqlite3 *db;

int use_hd_db(void);

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
    char query[1024];

    snprintf(query, sizeof(query), 
        "CREATE TABLE IF NOT EXISTS %s ( \
             id    INTEGER PRIMARY KEY, \
             time  DATE, \
             value FLOAT \
         )", name);

    return exec_query(query);
}

float
get_value(char *table)
{
    return get_row("value", table);
}

float
get_row(char *row, char *table)
{
    char          query[1024];
    float         value;
    sqlite3_stmt  *stmt;
    int           ret;

    snprintf(query, sizeof(query),
            "SELECT %s FROM %s ORDER BY id DESC LIMIT 1",
            row, table);

#ifdef DEBUG_SQLITE
    printf("sql: %s\n", query);
#endif

    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) != SQLITE_OK)
    {
        printf("couldn't execute query: '%s'\n", query);
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

        return get_row(row, table);
        printf("SUCCESSFULLY RETRIED!\n");

        return 0;
#else
        return get_row(row, table);
#endif

    }

    if (ret == SQLITE_ROW)
        value = atof((const char *) sqlite3_column_text(stmt, 0));
    else
        value = -1;

    sqlite3_finalize(stmt);

    return value;
}

float
get_average(char *row, char *table, int time)
{
    char          query[1024];
    float         average;
    sqlite3_stmt  *res;

    if (time > 0)
    {
        snprintf(query, sizeof(query),
            "SELECT AVG (%s) FROM %s WHERE time > DATETIME('NOW', '-%d minutes')",
            row, table, time);
    }
    else
    {
        snprintf(query, sizeof(query),
            "SELECT AVG (%s) FROM %s",
            row, table);
    }

#ifdef DEBUG_SQLITE
    printf("sql: %s\n", query);
#endif

    if (sqlite3_prepare_v2(db, query, strlen(query), &res, NULL) != SQLITE_OK)
    {
        printf("couldn't execute query: '%s'\n", query);
        return -1;
    }

    if (sqlite3_step(res) == SQLITE_ROW)
    {
        if (EOF == sscanf( (const char * __restrict__) sqlite3_column_text(res, 0), "%f", &average) )
            average = -1;
    }
    else
        average = -1;

    sqlite3_finalize(res);

    return average;
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

    buf = malloc(sizeof(void) * 256);

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

