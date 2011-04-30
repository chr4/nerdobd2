#include "httpd.h"
#include <sqlite3.h>

sqlite3 *db;


static int
busy(void *unused __attribute__((unused)), int count)
{
	usleep(500000);
    puts("retrying query...\n");
    
    // give up after 15 seconds (30 iterations)
	return (count < 30);
}


int
open_db(void)
{
    if (access(DB_RAM, R_OK) != 0)
    {
        perror("could not open database");
        return -1;
    }
        
    if (sqlite3_open(DB_RAM, &db) != 0)
    {
        perror("could not open database");
        return -1;
    }
    
    // retry on busy errors
    sqlite3_busy_handler(db, busy, NULL);
    
    return 0;
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
json_get_engine_data(json_object *data)
{
    char          query[LEN_QUERY];
    sqlite3_stmt  *stmt;
    
    // query engine data
    snprintf(query, sizeof(query),
             "SELECT rpm, speed, injection_time, \
             oil_pressure, consumption_per_100km, consumption_per_h \
             FROM engine_data \
             ORDER BY id \
             DESC LIMIT 1");
    
    
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
        add_double(data, "rpm", sqlite3_column_double(stmt, 0));
        add_double(data, "speed", sqlite3_column_double(stmt, 1));
        add_double(data, "injection_time", sqlite3_column_double(stmt, 2));
        add_double(data, "oil_pressure", sqlite3_column_double(stmt, 3));
        add_double(data, "consumption_per_100km", sqlite3_column_double(stmt, 4));
        add_double(data, "consumption_per_h", sqlite3_column_double(stmt, 5));
    }
    
    if (sqlite3_finalize(stmt) != SQLITE_OK)
    {
        printf("sqlite3_finalize() error\n");
        return -1;
    }  
    
    return 0;
}


int
json_get_other_data(json_object *data)
{
    char          query[LEN_QUERY];
    sqlite3_stmt  *stmt;
    
    // query other data
    snprintf(query, sizeof(query),
             "SELECT temp_engine, temp_air_intake, voltage \
             FROM other_data \
             ORDER BY id \
             DESC LIMIT 1");
    
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
        add_double(data, "temp_engine", sqlite3_column_double(stmt, 0));
        add_double(data, "temp_air_intake", sqlite3_column_double(stmt, 1));
        add_double(data, "voltage", sqlite3_column_double(stmt, 2));  
    }
    
    if (sqlite3_finalize(stmt) != SQLITE_OK)
    {
        printf("sqlite3_finalize() error\n");
        return -1;
    }   

    return 0;
}


int
json_get_averages(json_object *data, unsigned long int timespan)
{
    char          query[LEN_QUERY];
    sqlite3_stmt  *stmt;
    
    /* TODO: averages from current timespan displayed
     averages since last manual reset
     averages since beginning of calculation
     */
    snprintf(query, sizeof(query),
             "SELECT SUM(speed*consumption_per_100km)/SUM(speed) \
             FROM engine_data \
             WHERE time > DATETIME('NOW', '-%lu seconds') \
             AND consumption_per_100km != -1",
             timespan);
    
    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) != SQLITE_OK)
    {
        printf("couldn't execute query: '%s'\n", query);
        return -1;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW)
        add_double(data, "consumption_average_timespan", sqlite3_column_double(stmt, 0));
    
    if (sqlite3_finalize(stmt) != SQLITE_OK)
    {
        printf("sqlite3_finalize() error\n");
        return -1;
    }
    
    
    // get the overall consumption average
    snprintf(query, sizeof(query),
             "SELECT SUM(speed*consumption_per_100km)/SUM(speed) \
             FROM engine_data \
             WHERE consumption_per_100km != -1");
    
    if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL) != SQLITE_OK)
    {
        printf("couldn't execute query: '%s'\n", query);
        return -1;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW)
        add_double(data, "consumption_average_total", sqlite3_column_double(stmt, 0));
    
    if (sqlite3_finalize(stmt) != SQLITE_OK)
    {
        printf("sqlite3_finalize() error\n");
        return -1;
    }    
    
    return 0;
}


// get latest data from database
const char *
json_latest_data(unsigned long int timespan)
{   
    json_object *data = json_object_new_object();
    
    exec_query("BEGIN TRANSACTION");
    
    json_get_engine_data(data);
    json_get_other_data(data);
    json_get_averages(data, timespan);
    
    exec_query("END TRANSACTION");
    
    return json_object_to_json_string(data);
}


/*
 * get json data for graphing table key
 * getting all data since id index
 * but not older than timepsan seconds
 */
const char *
json_graph_data(char *key, unsigned long int index, unsigned long int timespan)
{
    char          query[LEN_QUERY];
    sqlite3_stmt  *stmt;
    
    json_object *graph = json_object_new_object();
    json_object *data = add_array(graph, "data");

    exec_query("BEGIN TRANSACTION");
    
    snprintf(query, sizeof(query),    
             "SELECT id, strftime('%%s000', time), %s \
              FROM   engine_data \
              WHERE id > %lu \
              AND time > DATETIME('NOW', '-%lu seconds') \
              ORDER BY id",
             key, index, timespan);
    
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
        printf("sqlite3_finalize() error\n");
        return NULL;
    }
    
    exec_query("END TRANSACTION");
    
    add_int(graph, "index", index);

    return json_object_to_json_string(graph);
}