pid_t  ajax;

// spawn ajax server 
if ( (ajax = fork()) == 0)
{
    // remove signal handlers
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    
    ajax_socket(8080);
    _exit(0);
}




// get averages
json_object *
json_averages(int timespan)
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
             WHERE time > DATETIME('NOW', '-%d minutes') \
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
            add_double(averages, "new", sqlite3_column_double(stmt, 0));
        }
    } while(ret != SQLITE_DONE);
    
    sqlite3_finalize(stmt);
    
    return averages;
}

// get latest engine data from database
json_object *
json_latest_data(void)
{
    char          query[LEN_QUERY];
    sqlite3_stmt  *stmt;
    int           ret;
    
    json_object *data = json_object_new_object();
    
    snprintf(query, sizeof(query),
             "SELECT   engine_data.*, \
             voltage.value, temp_engine.value, temp_air_intake.value \
             FROM     engine_data, voltage, temp_engine, temp_air_intake \
             ORDER BY id \
             DESC LIMIT 1");
    
    
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
            add_double(data, "rpm", sqlite3_column_double(stmt, 2));
            add_double(data, "speed", sqlite3_column_double(stmt, 3));
            add_double(data, "injection_time", sqlite3_column_double(stmt, 4));
            add_double(data, "oil_pressure", sqlite3_column_double(stmt, 5));
            add_double(data, "per_km", sqlite3_column_double(stmt, 6));
            add_double(data, "per_h", sqlite3_column_double(stmt, 7));
            add_double(data, "voltage", sqlite3_column_double(stmt, 8));
            add_double(data, "temp_engine", sqlite3_column_double(stmt, 9));
            add_double(data, "temp_air_intake", sqlite3_column_double(stmt, 10));
        }
    } while(ret != SQLITE_DONE);
    
    sqlite3_finalize(stmt);
    
    return data;
}

json_object *
json_generate_graph(char *key, int span)
{
    char          query[LEN_QUERY];
    sqlite3_stmt  *stmt;
    int           ret;
    int           index = 0;
    
    json_object *graph = json_object_new_object();
    json_object *data = add_array(graph, "data");
    
    snprintf(query, sizeof(query),
             "SELECT id, %s, strftime('%%s000', time) \
             FROM   engine_data \
             WHERE id > %d \
             ORDER BY time",
             key, span);
    
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
            add_data(data, sqlite3_column_double(stmt, 2),
                     sqlite3_column_double(stmt, 1));
            
            index = sqlite3_column_int(stmt, 0);
        }
        
    } while(ret != SQLITE_DONE);
    
    sqlite3_finalize(stmt);
    
    add_int(graph, "index", index);
    
    return graph;
}

const char *
json_generate(int span_consumption, int span_speed, int timespan)
{
    json_object *json = json_object_new_object();
    
    // get latest engine data from database
    json_object_object_add(json, "latest_data", json_latest_data());    
    
    // get averages
    json_object_object_add(json, "averages", json_averages(timespan));
    
    // graphing data 
    json_object_object_add(json, "graph_consumption", json_generate_graph("per_km", span_consumption));
    json_object_object_add(json, "graph_speed", json_generate_graph("speed", span_speed));
    
    return json_object_to_json_string(json);
}
