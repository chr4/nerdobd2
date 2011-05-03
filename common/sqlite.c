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


sqlite3 *
init_db(void)
{
    sqlite3 *db;
    
    // open database file
    if (sqlite3_open(DB_DISK, &db) != 0)
    {
        printf("Can not open database: %s", DB_DISK);
        return NULL;
    }

    // retry on busy errors
    sqlite3_busy_handler(db, busy, NULL);

    exec_query(db, "BEGIN TRANSACTION");

    // disable waiting for write to be completed
    exec_query(db, "PRAGMA synchronous = OFF");

    // disable journal
    exec_query(db, "PRAGMA journal_mode = OFF");

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
    
    return db;
}



void
close_db(sqlite3 *db)
{
    sqlite3_close(db);
}

