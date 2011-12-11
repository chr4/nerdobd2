#include "httpd.h"
#include <math.h>

// calls add_double, but checks if value actually is a number.
void
_add_double(json_object *parent, char *key, sqlite3_stmt *stmt, int column)
{
    if (sqlite3_column_type(stmt, column) == SQLITE_FLOAT)
        add_double(parent, key, sqlite3_column_double(stmt, column));
    else
        add_string(parent, key, "null"); // json doesn't support NaN
}

int
json_get_data(sqlite3 *db, json_object *data)
{
    char          query[LEN_QUERY];
    sqlite3_stmt  *stmt;

    // query data
    snprintf(query, sizeof(query),
             "SELECT rpm, speed, injection_time, \
                     oil_pressure, consumption_per_100km, consumption_per_h, \
                     temp_engine, temp_air_intake, voltage, \
                     gps_mode, \
                     gps_latitude, gps_longitude, gps_altitude, \
                     gps_speed, gps_climb, gps_track, \
                     gps_err_latitude, gps_err_longitude, gps_err_altitude, \
                     gps_err_speed, gps_err_climb, gps_err_track \
              FROM data \
              ORDER BY id \
              DESC LIMIT 1");

#ifdef DEBUG_SQLITE
    printf("sql: %s\n", query);
#endif

    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) != SQLITE_OK)
    {
        printf("couldn't execute query: '%s'\n", query);
        return -1;
    }

    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) != SQLITE_OK)
    {
        printf("couldn't execute query: '%s'\n", query);
        return -1;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        _add_double(data, "rpm", stmt, 0);
        _add_double(data, "speed", stmt, 1);
        _add_double(data, "injection_time", stmt, 2);
        _add_double(data, "oil_pressure", stmt, 3);
        _add_double(data, "consumption_per_100km", stmt, 4);
        _add_double(data, "consumption_per_h", stmt, 5);
        _add_double(data, "temp_engine", stmt, 6);
        _add_double(data, "temp_air_intake", stmt, 7);
        _add_double(data, "voltage", stmt, 8);
        _add_double(data, "gps_mode", stmt, 9);
        _add_double(data, "gps_latitude", stmt, 10);
        _add_double(data, "gps_longitude", stmt, 11);
        _add_double(data, "gps_altitude", stmt, 12);
        _add_double(data, "gps_speed", stmt, 13);
        _add_double(data, "gps_climb", stmt, 14);
        _add_double(data, "gps_track", stmt, 15);
        _add_double(data, "gps_err_latitude", stmt, 16);
        _add_double(data, "gps_err_longitude", stmt, 17);
        _add_double(data, "gps_err_altitude", stmt, 18);
        _add_double(data, "gps_err_speed", stmt, 19);
        _add_double(data, "gps_err_climb", stmt, 20);
        _add_double(data, "gps_err_track", stmt, 21);
    }
    if (sqlite3_finalize(stmt) != SQLITE_OK)
    {
#ifdef DEBUG_SQLITE
        printf("sqlite3_finalize() error\n");
#endif
        return -1;
    }

    return 0;
}


int
json_get_averages(sqlite3 *db, json_object *data)
{
    char          query[LEN_QUERY];
    sqlite3_stmt  *stmt;

    // average since last startup
    snprintf(query, sizeof(query),
             "SELECT strftime('%%s000', setpoints.time), \
                     SUM(data.speed * data.consumption_per_100km) / SUM(data.speed), \
                     SUM(data.liters), \
                     SUM(data.kilometers) \
              FROM   setpoints, data \
              WHERE  consumption_per_100km != -1 \
              AND    id > ( SELECT data FROM setpoints WHERE name = 'startup' )");

#ifdef DEBUG_SQLITE
    printf("sql: %s\n", query);
#endif
    
    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) != SQLITE_OK)
    {
        printf("couldn't execute query: '%s'\n", query);
        return -1;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        _add_double(data, "timestamp_startup", stmt, 0);
        _add_double(data, "consumption_average_startup", stmt, 1);
        _add_double(data, "consumption_liters_startup", stmt, 2);
        _add_double(data, "kilometers_startup", stmt, 3);        
    }
    
    if (sqlite3_finalize(stmt) != SQLITE_OK)
    {
#ifdef DEBUG_SQLITE
        printf("sqlite3_finalize() error\n");
#endif
        return -1;
    }

    // average since timespan
    /*
    snprintf(query, sizeof(query),
             "SELECT SUM(speed*consumption_per_100km)/SUM(speed) \
             FROM data \
             WHERE time > DATETIME('NOW', '-%lu seconds', 'localtime') \
             AND consumption_per_100km != -1",
             timespan);
    */
    

    // overall consumption average
    snprintf(query, sizeof(query),
             "SELECT SUM(speed * consumption_per_100km) / SUM(speed), \
                     SUM(liters), \
                     SUM(kilometers) \
              FROM   data \
              WHERE  consumption_per_100km != -1");

#ifdef DEBUG_SQLITE
    printf("sql: %s\n", query);
#endif
    
    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) != SQLITE_OK)
    {
        printf("couldn't execute query: '%s'\n", query);
        return -1;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        _add_double(data, "consumption_average_total", stmt, 0);
        _add_double(data, "consumption_liters_total", stmt, 1);
        _add_double(data, "kilometers_total", stmt, 2);
    }
    
    if (sqlite3_finalize(stmt) != SQLITE_OK)
    {
#ifdef DEBUG_SQLITE
        printf("sqlite3_finalize() error\n");
#endif
        return -1;
    }

    return 0;
}


// get latest data from database
const char *
json_latest_data(sqlite3 *db)
{
    json_object *data = json_object_new_object();

    exec_query(db, "BEGIN TRANSACTION");

    json_get_data(db, data);
    json_get_averages(db, data);

    exec_query(db, "END TRANSACTION");

    return json_object_to_json_string(data);
}


/*
 * get json data for graphing table key
 * getting all data since id index
 * but not older than timepsan seconds
 */
const char *
json_graph_data(sqlite3 *db, char *key, unsigned long int index, unsigned long int timespan)
{
    char          query[LEN_QUERY];
    sqlite3_stmt  *stmt;

    json_object *graph = json_object_new_object();
    json_object *data = add_array(graph, "data");

    exec_query(db, "BEGIN TRANSACTION");

    snprintf(query, sizeof(query),
             "SELECT id, strftime('%%s000', time), %s \
              FROM   data \
              WHERE id > %lu \
              AND time > DATETIME('NOW', '-%lu seconds', 'localtime') \
              ORDER BY id",
             key, index, timespan);

#ifdef DEBUG_SQLITE
    printf("sql: %s\n", query);
#endif
    
    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) != SQLITE_OK)
    {
        printf("couldn't execute query: '%s'\n", query);
        return NULL;
    }

    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) != SQLITE_OK)
    {
        printf("couldn't execute query: '%s'\n", query);
        return NULL;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        add_data(data, sqlite3_column_double(stmt, 1),
                       sqlite3_column_double(stmt, 2));

        index = sqlite3_column_int(stmt, 0);
    }

    if (sqlite3_finalize(stmt) != SQLITE_OK)
    {
#ifdef DEBUG_SQLITE
        printf("sqlite3_finalize() error\n");
#endif
        return NULL;
    }

    exec_query(db, "END TRANSACTION");

    add_int(graph, "index", index);

    return json_object_to_json_string(graph);
}
