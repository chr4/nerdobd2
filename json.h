#include <json/json.h>

// json helper functions
json_object *add_string(json_object *, char *, char *);
json_object *add_int(json_object *, char *, int);
json_object *add_double(json_object *, char *, double);
json_object *add_boolean(json_object *, char *, char);
json_object *add_array(json_object *, char *);
json_object *add_object(json_object *, char *);
json_object *add_data(json_object *, double, double);
