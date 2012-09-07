#include "httpd.h"
#include <math.h>

// calls add_double, but checks if value actually is a number.
void
_add_double(json_object * parent, char *key, PGresult * res, int column) {
    float   f;
    f = atof(PQgetvalue(res, 0, column));

    if (isnan(f))
        add_string(parent, key, "null");        // json doesn't support NaN
    else
        add_double(parent, key, atof(PQgetvalue(res, 0, column)));
}

int
json_query_and_add(PGconn * db, char *query, json_object * data) {
    PGresult *res;
    int     i;

#ifdef DEBUG_DB
    printf("sql: %s\n", query);
#endif

    res = PQexec(db, query);

    switch (PQresultStatus(res)) {

        case PGRES_TUPLES_OK:
            if (PQntuples(res) > 0)
                for (i = 0; i < PQnfields(res); i++)
                    _add_double(data, PQfname(res, i), res, i);

            PQclear(res);
            return 0;
            break;

        case PGRES_COMMAND_OK:
            // all OK, no data to process
            break;

        case PGRES_EMPTY_QUERY:
            // server had nothing to do, a bug maybe?
            fprintf(stderr, "query '%s' failed (EMPTY_QUERY): %s\n", query,
                    PQerrorMessage(db));
            break;

        case PGRES_NONFATAL_ERROR:
            // can continue, possibly retry the command
            fprintf(stderr, "query '%s' failed (NONFATAL, retrying): %s\n",
                    query, PQerrorMessage(db));
            PQclear(res);
            return json_query_and_add(db, query, data);
            break;

        case PGRES_BAD_RESPONSE:
        case PGRES_FATAL_ERROR:
        default:
            // fatal or unknown error, cannot continue
            fprintf(stderr, "query '%s' failed: %s\n", query,
                    PQerrorMessage(db));
    }

    PQclear(res);
    return -1;
}

const char *
json_get_data(PGconn * db) {
    json_object *data = json_object_new_object();

    json_query_and_add(db, "SELECT rpm, speed, injection_time, \
                oil_pressure, consumption_per_100km, consumption_per_h, \
                temp_engine, temp_air_intake, voltage, \
                gps_mode, \
                gps_latitude, gps_longitude, gps_altitude, \
                gps_speed, gps_climb, gps_track, \
                gps_err_latitude, gps_err_longitude, gps_err_altitude, \
                gps_err_speed, gps_err_climb, gps_err_track \
         FROM data \
         ORDER BY id \
         DESC LIMIT 1", data);

    return json_object_to_json_string(data);
}


// this functino is VERY ressource heavy, needs to be improved
const char *
json_get_averages(PGconn * db) {
    json_object *data = json_object_new_object();

    // average since last startup
    if (json_query_and_add(db,
                           "SELECT   date_part('epoch', setpoints.time) AS timestamp_startup, \
                       SUM(data.speed * data.consumption_per_100km) / SUM(data.speed) AS consumption_average_startup, \
                       SUM(data.liters) AS consumption_liters_startup, \
                       SUM(data.kilometers) AS kilometers_startup \
              FROM     setpoints, data \
              WHERE    consumption_per_100km != 'NaN' \
              AND      data.id > ( SELECT data FROM setpoints WHERE name = 'startup' ) \
              GROUP BY setpoints.time",
                           data) == -1)
        return NULL;

    // average since timespan
    /*
     * snprintf(query, sizeof(query),
     * "SELECT SUM(speed*consumption_per_100km)/SUM(speed) \
     * FROM data \
     * WHERE time > current_timestamp - interval '%lu seconds' \
     * AND consumption_per_100km != 'NaN'",
     * timespan);
     */


    // overall consumption average
    if (json_query_and_add(db,
                           "SELECT SUM(speed * consumption_per_100km) / SUM(speed) AS consumption_average_total, \
                     SUM(liters) AS consumption_liters_total, \
                     SUM(kilometers) AS kilometers_total \
              FROM   data \
              WHERE  consumption_per_100km != 'NaN'",
                           data) == -1)
        return NULL;

    return json_object_to_json_string(data);
}


/*
 * get json data for graphing table key
 * getting all data since id index
 * but not older than timepsan seconds
 */
const char *
json_graph_data(PGconn * db, char *key, unsigned long int index,
                unsigned long int timespan) {
    char    query[LEN_QUERY];
    PGresult *res;
    double  timestamp;
    float   value;
    int     i;

    json_object *graph = json_object_new_object();
    json_object *data = add_array(graph, "data");

    snprintf(query, sizeof(query),
             "SELECT date_part('epoch', time) * 1000, %s \
              FROM   data \
              WHERE id > %lu \
              AND time > current_timestamp - interval '%lu seconds' \
              ORDER BY id", key, index, timespan);

#ifdef DEBUG_DB
    printf("sql: %s\n", query);
#endif

    res = PQexec(db, query);

    switch (PQresultStatus(res)) {

        case PGRES_TUPLES_OK:
            for (i = 0; i < PQntuples(res); i++) {
                timestamp = atof(PQgetvalue(res, i, 0));
                value = atof(PQgetvalue(res, i, 1));
                if (isnan(value))
                    continue;

                add_data(data, timestamp, value);
                add_int(graph, "index", index);
            }

            PQclear(res);
            return json_object_to_json_string(graph);
            break;

        case PGRES_COMMAND_OK:
            // all OK, no data to process
            break;

        case PGRES_EMPTY_QUERY:
            // server had nothing to do, a bug maybe?
            fprintf(stderr, "query '%s' failed (EMPTY_QUERY): %s\n", query,
                    PQerrorMessage(db));
            break;

        case PGRES_NONFATAL_ERROR:
            // can continue, possibly retry the command
            fprintf(stderr, "query '%s' failed (NONFATAL, retrying): %s\n",
                    query, PQerrorMessage(db));
            PQclear(res);
            return json_graph_data(db, key, index, timespan);
            break;

        case PGRES_BAD_RESPONSE:
        case PGRES_FATAL_ERROR:
        default:
            // fatal or unknown error, cannot continue
            fprintf(stderr, "query '%s' failed: %s\n", query,
                    PQerrorMessage(db));
    }

    PQclear(res);
    return NULL;
}
