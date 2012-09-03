#include "json.h"
#include <math.h>

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
    json_object *obj_value;

    // json doesn't support NaN
    if (isnan(value))
        obj_value = json_object_new_string("null");
    else
        obj_value = json_object_new_double(value);

    json_object_array_add(array, obj_label);
    json_object_array_add(array, obj_value);

    json_object_array_add(parent, array);

    return array;
}

