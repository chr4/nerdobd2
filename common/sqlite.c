#include "sqlite.h"

// the database handler
sqlite3 *db;

int use_hd_db(void);

static int
busy(void *unused __attribute__((unused)), int count)
{
	usleep(500000);
    puts("retrying query...\n");

    // give up after 30 seconds
	return (count < 60);
}

int
exec_query(char *query)
{

    sqlite3_stmt  *stmt;

#ifdef DEBUG_SQLITE
    printf("sql: %s\n", query);
#endif

    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) != SQLITE_OK)
    {
        printf("sqlite3_prepare_v2() error\n");
        return -1;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        printf("sqlite3_step error\n");
        return -1;
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK)
    {
        printf("sqlite3_finalize() error\n");
        return -1;
    }

    return 0;
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
    if (sqlite3_open(DB_RAM, &db) != 0)
    {
        printf("Can not open database: %s", DB_RAM);
        return -1;
    }

    // retry on busy errors
    sqlite3_busy_handler(db, busy, NULL);

    exec_query("BEGIN TRANSACTION");

    // disable waiting for write to be completed
    exec_query("PRAGMA synchronous = OFF");

    // disable journal
    exec_query("PRAGMA journal_mode = OFF");

    // create engine_data table
    exec_query("CREATE TABLE IF NOT EXISTS engine_data ( \
                    id                    INTEGER PRIMARY KEY, \
                    time                  DATE, \
                    rpm                   FLOAT, \
                    speed                 FLOAT, \
                    injection_time        FLOAT, \
                    oil_pressure          FLOAT, \
                    consumption_per_100km FLOAT, \
                    consumption_per_h     FLOAT )");

    // create other_data table
    exec_query("CREATE TABLE IF NOT EXISTS other_data ( \
                    id              INTEGER PRIMARY KEY, \
                    time            DATE, \
                    temp_engine     FLOAT, \
                    temp_air_intake FLOAT, \
                    voltage         FLOAT )");
    
    // create table where set point information is stored
    exec_query("CREATE TABLE IF NOT EXISTS setpoints ( \
                    name        VARCHAR PRIMARY KEY, \
                    engine_data INTEGER)");
    
    exec_query("END TRANSACTION");
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
        if (write (ram, buf, n) <= 0)
        {
            perror("error while copying file");
            return -1;
        }
    }
    while (n);

    close(hd);
    close(ram);

    return 0;
}
