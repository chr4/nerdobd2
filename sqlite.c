#include "sqlite.h"

static int
busy(void *unused __attribute__ ((unused)), int count) {
    usleep(500000);

#ifdef DEBUG_DB
    printf("retrying query...\n");
#endif

    // give up after 30 seconds
    return (count < 60);
}


int
exec_query(sqlite3 * db, char *query) {
    char   *error = NULL;

#ifdef DEBUG_DB
    if (strstr(query, "TRANSACTION") == NULL)
        printf("sql: %s\n", query);
#endif

    if (sqlite3_exec(db, query, NULL, NULL, &error) != SQLITE_OK) {
        printf("sql error: %s\n", error);
        sqlite3_free(error);
        return -1;
    }

    return 0;
}


sqlite3 *
open_db(void) {
    sqlite3 *db;

    // open database file
    if (sqlite3_open(DB_SQLITE, &db) != SQLITE_OK) {
        printf("Coudln't open database: %s", DB_SQLITE);
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
init_db(sqlite3 * db) {
    exec_query(db, "BEGIN TRANSACTION");

    // create data table
    exec_query(db, "CREATE TABLE IF NOT EXISTS data ( \
                        id                    INTEGER PRIMARY KEY, \
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
                        name        VARCHAR PRIMARY KEY, \
                        time        DATE, \
                        data        INTEGER)");

    exec_query(db, "END TRANSACTION");

    return;
}



void
close_db(sqlite3 * db) {
    sqlite3_close(db);
}
