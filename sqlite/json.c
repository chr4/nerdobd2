#include "sqlite.h"
#include "../common/config.h"

json_object *
add_string(json_object *parent, char *key, char *value)
{
    json_object *new_obj = json_object_new_string(value);
    json_object_object_add(parent, key, new_obj);

    return new_obj;
}  

json_object *
add_int(json_object *parent, char *key, int value)
{
    json_object *new_obj = json_object_new_int(value);
    json_object_object_add(parent, key, new_obj);

    return new_obj;
}

json_object *
add_double(json_object *parent, char *key, double value)
{
    json_object *new_obj = json_object_new_double(value);
    json_object_object_add(parent, key, new_obj);

    return new_obj;
}

json_object *
add_boolean(json_object *parent, char *key, char boolean)
{
    json_object *new_obj = json_object_new_boolean(boolean);
    json_object_object_add(parent, key, new_obj);

    return new_obj;
}

json_object *
add_array(json_object *parent, char *key)
{
    json_object *new_obj = json_object_new_array();
    json_object_object_add(parent, key, new_obj);

    return new_obj;
}

json_object *
add_object(json_object *parent, char *key)
{
    json_object *new_obj = json_object_new_object();

    if (key == NULL)
        json_object_array_add(parent, new_obj);
    else
        json_object_object_add(parent, key, new_obj);

    return new_obj;
}

json_object *
add_data(json_object *parent, double label, double value)
{
    json_object *array     = json_object_new_array();

    json_object *obj_label = json_object_new_double(label);
    json_object *obj_value = json_object_new_double(value);

    json_object_array_add(array, obj_label);
    json_object_array_add(array, obj_value);

    json_object_array_add(parent, array);

    return array;
}


const char *
json_generate_graph(char *key, char *table, int minutes)
{
    char          query[1024];
    sqlite3_stmt  *stmt;
    int           ret;


    json_object *json = json_object_new_object();

    // options
    /* those are set in javascript
    json_object *options = add_object(json, "options");
    add_boolean( add_object(options, "lines"), "show", 1);
    add_int( add_object(options, "series"), "shadowSize", 0);

    json_object *yaxis = add_object(options, "yaxis");
    add_int(yaxis, "min", 0);
    add_int(yaxis, "max", 160);

    json_object *xaxis = add_object(options, "xaxis");
    add_string(xaxis, "mode", "time");
    add_string(xaxis, "timeformat", "%H:%M");
    */

    // data
    add_string(json, "label", "speed");
    json_object *data = add_array(json, "data");

    snprintf(query, sizeof(query),
             "SELECT %s, strftime('%%s000', time) FROM %s WHERE time > DATETIME('NOW', '-%d minutes') ORDER BY time",
            key, table, minutes);

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
            printf("database busy.\n");
            // wait for 0.5 sec
            usleep(500000);
            continue;
        } 

        if (ret == SQLITE_ROW) {
            add_data(data, sqlite3_column_double(stmt, 1),
                           sqlite3_column_double(stmt, 0));

            /* those are set in javascript
            // display range from newest dataset to dataset 5 minutes ago.
            // javascript uses not seconds, but milliseconds, so * 1000
            add_double(xaxis, "min", sqlite3_column_double(stmt, 1) - 500000);
            add_double(xaxis, "max", sqlite3_column_double(stmt, 1));
            */
        }

    } while(ret != SQLITE_DONE);

    sqlite3_finalize(stmt);

  return json_object_to_json_string(json);
}

