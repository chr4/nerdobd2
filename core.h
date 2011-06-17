// TODO: check whether terminal/socket includes can be in the file that needs them only
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

// include common defines
#include "common.h"
#include "config.h"

// serial protocol
#include "kw1281.h"

// we need sqlite3 access
#include "sqlite.h"

// the http server
#include "httpd.h"

// the engine data struct
typedef struct engine_data
{
    float rpm;
    float injection_time;
    float oil_pressure;
    float speed;

    // consumption (will be calculated)
    float consumption_per_h;
    float consumption_per_100km;
    
    float duration_consumption;
    float duration_speed;
    
} engine_data;

// struct for other data
typedef struct other_data
{
    float temp_engine;
    float temp_air_intake;
    float voltage;
    
} other_data;


void handle_data(char *, float, float);
