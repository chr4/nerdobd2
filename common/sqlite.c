#include "sqlite.h"


static int
busy(void *unused __attribute__((unused)), int count)
{
	usleep(500000);
    puts("retrying query...\n");

    // give up after 30 seconds
	return (count < 60);
}


int
exec_query(sqlite3 *db, char *query)
{
    char *error = NULL;
    
#ifdef DEBUG_SQLITE
    if (strstr(query, "TRANSACTION") == NULL)
        printf("sql: %s\n", query);
#endif
    
    if (sqlite3_exec(db, query, NULL, NULL, &error) != SQLITE_OK)
    {
        printf("sql error: %s\n", error);       
        sqlite3_free(error);
        return -1;
    }
    
    return 0;
}


// sync database in ram to disk
void
sync2disk(void)
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


// sync database in disk to ram
int
sync2ram(void)
{
    int      hd;
    int      ram;
    int      n;
    void    *buf;
    sqlite3 *db;
    
    
    /* check if database file exists in /dev/shm
     * if not, look for one on the disk and use it
     * if nothing is found, create a new one.
     */
    if (access(DB_RAM, W_OK) == 0)
    {
#ifdef DEBUG_SQLITE
        printf("%s exists and is writeable, no sync needed\n", DB_RAM);
#endif
        return 0;        
    }
    else if (access(DB_DISK, R_OK) == 0)
    {
#ifdef DEBUG_SQLITE
        printf("%s exists and is readable, syncing to ram...\n", DB_DISK);
#endif
    }
    else
    {
#ifdef DEBUG_SQLITE
        printf("no db file found, creating...\n");
#endif
        // create database file
        if (sqlite3_open(DB_RAM, &db) != SQLITE_OK)
        {
            printf("Coudln't create database: %s", DB_RAM);
            return -1;
        }
        close_db(db);
        
        return 0;
    }    
    
    
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
        if (write (ram, buf, n) == -1)
        {
            printf("error while copying file. (write() error)\n");
            return -1;
        }
    }
    while (n);
    
    close(hd);
    close(ram);
    
    return 0;
}


sqlite3 * 
open_db(void)
{
    sqlite3 *db;
    
    if (access(DB_RAM, W_OK) != 0)
    {
        printf("Couldn't open database: %s\n", DB_RAM);
        return NULL;
    }
    
    // open database file
    if (sqlite3_open(DB_RAM, &db) != SQLITE_OK)
    {
        printf("Coudln't open database: %s", DB_RAM);
        return NULL;
    }
    
    // retry on busy errors
    sqlite3_busy_handler(db, busy, NULL);
        
    // disable waiting for write to be completed
    exec_query(db, "PRAGMA synchronous = OFF");
    
    // disable journal
    exec_query(db, "PRAGMA journal_mode = OFF");
    
    return db;
}
        

void
init_db(sqlite3 *db)
{
    exec_query(db, "BEGIN TRANSACTION");

    // create engine_data table
    exec_query(db, "CREATE TABLE IF NOT EXISTS engine_data ( \
                        id                    INTEGER PRIMARY KEY, \
                        time                  DATE, \
                        rpm                   FLOAT, \
                        speed                 FLOAT, \
                        injection_time        FLOAT, \
                        oil_pressure          FLOAT, \
                        consumption_per_100km FLOAT, \
                        consumption_per_h     FLOAT )");

    // create other_data table
    exec_query(db, "CREATE TABLE IF NOT EXISTS other_data ( \
                        id              INTEGER PRIMARY KEY, \
                        time            DATE, \
                        temp_engine     FLOAT, \
                        temp_air_intake FLOAT, \
                        voltage         FLOAT )");
    
    // create table where set point information is stored
    exec_query(db, "CREATE TABLE IF NOT EXISTS setpoints ( \
                        name        VARCHAR PRIMARY KEY, \
                        engine_data INTEGER)");
    
    exec_query(db, "END TRANSACTION");
    
    return;
}



void
close_db(sqlite3 *db)
{
    sqlite3_close(db);
}
