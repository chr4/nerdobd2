#include "httpd.h"
#include <math.h>

// calls add_double, but checks if value actually is a number.
void
_add_double(json_object *parent, char *key, PGresult *res, int column)
{
    float f;
    f = atof( PQgetvalue(res, 0, column) );

    if (isnan(f))
        add_string(parent, key, "null"); // json doesn't support NaN
    else
        add_double(parent, key, atof(PQgetvalue(res, 0, column)));
}

int
json_get_data(PGconn *db, json_object *data)
{
    char     query[LEN_QUERY];
    PGresult *res;

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

#ifdef DEBUG_DB
    printf("sql: %s\n", query);
#endif

    res = PQexec(db, query);   

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "query '%s' failed: %s\n", query, PQerrorMessage(db));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) > 0)
    {
        _add_double(data, "rpm", res, 0);
        _add_double(data, "speed", res, 1);
        _add_double(data, "injection_time", res, 2);
        _add_double(data, "oil_pressure", res, 3);
        _add_double(data, "consumption_per_100km", res, 4);
        _add_double(data, "consumption_per_h", res, 5);
        _add_double(data, "temp_engine", res, 6);
        _add_double(data, "temp_air_intake", res, 7);
        _add_double(data, "voltage", res, 8);
        _add_double(data, "gps_mode", res, 9);
        _add_double(data, "gps_latitude", res, 10);
        _add_double(data, "gps_longitude", res, 11);
        _add_double(data, "gps_altitude", res, 12);
        _add_double(data, "gps_speed", res, 13);
        _add_double(data, "gps_climb", res, 14);
        _add_double(data, "gps_track", res, 15);
        _add_double(data, "gps_err_latitude", res, 16);
        _add_double(data, "gps_err_longitude", res, 17);
        _add_double(data, "gps_err_altitude", res, 18);
        _add_double(data, "gps_err_speed", res, 19);
        _add_double(data, "gps_err_climb", res, 20);
        _add_double(data, "gps_err_track", res, 21);
    }

    PQclear(res);
    return 0;
}


int
json_get_averages(PGconn *db, json_object *data)
{
    char     query[LEN_QUERY];
    PGresult *res;

    // average since last startup
    snprintf(query, sizeof(query),
             "SELECT   date_part('epoch', setpoints.time), \
                       SUM(data.speed * data.consumption_per_100km) / SUM(data.speed), \
                       SUM(data.liters), \
                       SUM(data.kilometers) \
              FROM     setpoints, data \
              WHERE    consumption_per_100km != 'NaN' \
              AND      data.id > ( SELECT data FROM setpoints WHERE name = 'startup' ) \
              GROUP BY setpoints.time");

#ifdef DEBUG_DB
    printf("sql: %s\n", query);
#endif

    res = PQexec(db, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "query '%s' failed: %s\n", query, PQerrorMessage(db));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) > 0)
    {
        _add_double(data, "timestamp_startup", res, 0);
        _add_double(data, "consumption_average_startup", res, 1);
        _add_double(data, "consumption_liters_startup", res, 2);
        _add_double(data, "kilometers_startup", res, 3);        
    }

    PQclear(res);

    // average since timespan
    /*
    snprintf(query, sizeof(query),
             "SELECT SUM(speed*consumption_per_100km)/SUM(speed) \
             FROM data \
             WHERE time > DATETIME('NOW', '-%lu seconds', 'localtime') \
             AND consumption_per_100km != 'NaN'",
             timespan);
    */
    

    // overall consumption average
    snprintf(query, sizeof(query),
             "SELECT SUM(speed * consumption_per_100km) / SUM(speed), \
                     SUM(liters), \
                     SUM(kilometers) \
              FROM   data \
              WHERE  consumption_per_100km != 'NaN'");

#ifdef DEBUG_DB
    printf("sql: %s\n", query);
#endif

    res = PQexec(db, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "query '%s' failed: %s\n", query, PQerrorMessage(db));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) > 0)
    {
        _add_double(data, "consumption_average_total", res, 0);
        _add_double(data, "consumption_liters_total", res, 1);
        _add_double(data, "kilometers_total", res, 2);
    }
 
    PQclear(res);
    return 0;
}


// get latest data from database
const char *
json_latest_data(PGconn *db)
{
    json_object *data = json_object_new_object();

    json_get_data(db, data);
    json_get_averages(db, data);

    return json_object_to_json_string(data);
}


/*
 * get json data for graphing table key
 * getting all data since id index
 * but not older than timepsan seconds
 */
const char *
json_graph_data(PGconn *db, char *key, unsigned long int index, unsigned long int timespan)
{
    char      query[LEN_QUERY];
    PGresult  *res;
    float     *timestamp;
    float     *value;

    json_object *graph = json_object_new_object();
    json_object *data = add_array(graph, "data");

    snprintf(query, sizeof(query),
             "SELECT date_part('epoch', time), %s \
              FROM   data \
              WHERE id > %lu \
              AND time > DATETIME('NOW', '-%lu seconds', 'localtime') \
              ORDER BY id",
             key, index, timespan);

#ifdef DEBUG_DB
    printf("sql: %s\n", query);
#endif

    res = PQexec(db, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "query '%s' failed: %s\n", query, PQerrorMessage(db));
        PQclear(res);
        return NULL;
    }

    if (PQntuples(res) > 0)
    {
        timestamp = (float *) PQgetvalue(res, 0, 0);
        value = (float *) PQgetvalue(res, 0, 1);

        add_data(data, *timestamp, *value);
    }

    add_int(graph, "index", index);

    return json_object_to_json_string(graph);
}
