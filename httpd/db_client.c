#include "httpd.h"
#include <sqlite3.h>

sqlite3 *db;

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
    
    return 0;
}

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
        
    } while(ret != SQLITE_DONE);
    
    
    if (sqlite3_finalize(stmt) != SQLITE_OK)
        printf("sqlite3_finalize() error\n");
    
    return 0;
}


// get averages
json_object *
json_averages(unsigned long int timespan)
{
    char          query[LEN_QUERY];
    sqlite3_stmt  *stmt;
    int           ret;
    
    json_object *averages = json_object_new_object();
    
    /* TODO: averages from current timespan displayed
     averages since last manual reset
     averages since beginning of calculation
     */
    snprintf(query, sizeof(query),
             "SELECT SUM(speed*per_km)/SUM(speed), AVG(per_km) \
             FROM engine_data \
             WHERE time > DATETIME('NOW', '-%lu seconds') \
             AND per_km != -1",
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
            add_double(averages, "timespan_new", sqlite3_column_double(stmt, 0));
        }
    } while(ret != SQLITE_DONE);
    
    sqlite3_finalize(stmt);

    snprintf(query, sizeof(query),
             "SELECT SUM(speed*per_km)/SUM(speed), AVG(per_km) \
             FROM engine_data \
             WHERE per_km != -1");
    
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
            add_double(averages, "total", sqlite3_column_double(stmt, 1));
            add_double(averages, "total_new", sqlite3_column_double(stmt, 0));
        }
    } while(ret != SQLITE_DONE);
    
    sqlite3_finalize(stmt);    
    
    return averages;
}

// get latest data from database
json_object *
json_latest_data(void)
{
    char          query[LEN_QUERY];
    sqlite3_stmt  *stmt;
    int           ret;
    
    json_object *data = json_object_new_object();
    
    // query engine data
    snprintf(query, sizeof(query),
             "SELECT rpm, speed, injection_time, \
                     oil_pressure, per_km, per_h \
              FROM engine_data \
              ORDER BY id \
              DESC LIMIT 1");
  
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
            // wait for 0.5 sec
            usleep(500000);
            
#ifdef DEBUG_SQLITE
            printf("retrying query: %s\n", query);
#endif
            continue;
        }
        
        if (ret == SQLITE_ROW)
        {
            add_double(data, "rpm", sqlite3_column_double(stmt, 0));
            add_double(data, "speed", sqlite3_column_double(stmt, 1));
            add_double(data, "injection_time", sqlite3_column_double(stmt, 2));
            add_double(data, "oil_pressure", sqlite3_column_double(stmt, 3));
            add_double(data, "per_km", sqlite3_column_double(stmt, 4));
            add_double(data, "per_h", sqlite3_column_double(stmt, 5));

        }
    } while(ret != SQLITE_DONE);
    
    sqlite3_finalize(stmt);
   
    
    // query other data
    snprintf(query, sizeof(query),
             "SELECT temp_engine, temp_air_intake, voltage \
             FROM other_data \
             ORDER BY id \
             DESC LIMIT 1");
    
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
            // wait for 0.5 sec
            usleep(500000);
            
#ifdef DEBUG_SQLITE
            printf("retrying query: %s\n", query);
#endif
            continue;
        }
        
        if (ret == SQLITE_ROW)
        {
            add_double(data, "temp_engine", sqlite3_column_double(stmt, 0));
            add_double(data, "temp_air_intake", sqlite3_column_double(stmt, 1));
            add_double(data, "voltage", sqlite3_column_double(stmt, 2));  
        }
    } while(ret != SQLITE_DONE);

    sqlite3_finalize(stmt);

    
    return data;
}


/*
 * get json data for graphing table key
 * getting all data since id index
 * but not older than timepsan seconds
 */
json_object *
json_generate_graph(char *key, unsigned long int index, unsigned long int timespan)
{
    char          query[LEN_QUERY];
    sqlite3_stmt  *stmt;
    int           ret;
    
    json_object *graph = json_object_new_object();
    json_object *data = add_array(graph, "data");
    
    // average in groups, so we have max 470 entries
    // (we cannot display any more pixels)
    /*
    snprintf(query, sizeof(query),
             "SELECT id, SUM(strftime('%%s000', time))/count(1), \
              SUM(%s)/count(1) FROM engine_data \
              WHERE id > %lu \
              AND time > DATETIME('NOW', '-%lu seconds') \
              GROUP BY id / ( ( \
                  SELECT COUNT(id) FROM engine_data \
                  WHERE id > %lu \
                  AND time > DATETIME('NOW', '-%lu seconds') \
              ) / %d + 1 )",
             key, index, timespan, index, timespan, 100);
    */
    snprintf(query, sizeof(query),    
             "SELECT id, strftime('%%s000', time), %s \
              FROM   engine_data \
              WHERE id > %lu \
              AND time > DATETIME('NOW', '-%lu seconds') \
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
            add_data(data, sqlite3_column_double(stmt, 1),
                     sqlite3_column_double(stmt, 2));
            
            index = sqlite3_column_int(stmt, 0);
        }
        
    } while(ret != SQLITE_DONE);
    
    sqlite3_finalize(stmt);
    
    add_int(graph, "index", index);
    
    return graph;
}

const char *
json_generate(unsigned long int index_consumption, unsigned long int timespan_consumption,
              unsigned long int index_speed, unsigned long int timespan_speed)
{
    json_object *json = json_object_new_object();
    
    // statements are supposed to be faster when wrapped in one transaction
    exec_query("BEGIN TRANSACTION");
    
    // get latest engine data from database
    json_object_object_add(json, "latest_data", json_latest_data());    
    
    // get averages
    json_object_object_add(json, "averages", json_averages(timespan_speed));
    
    // graphing data 
    json_object_object_add(json, "graph_consumption", json_generate_graph("per_km", index_consumption, timespan_consumption));
    json_object_object_add(json, "graph_speed", json_generate_graph("speed", index_speed, timespan_speed));
    
    exec_query("END TRANSACTION");
    return json_object_to_json_string(json);
}
