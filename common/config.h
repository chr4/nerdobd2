#define DB_HOST	        "127.0.0.1"
#define DB_PORT         4588
#define HTTPD_PORT	8080
#define DOCROOT		"httpd"

//#define DEBUG_SERIAL
#define DEBUG_SQLITE
//#define DEBUG_AJAX
//#define TEST

/* writing to database is too slow on my eeepc,
 * thus provoking "resource temporarily not available" errors
 * putting database in /dev/shm, and syncing it from time to time
 * should help.
 */
#define DB_RAM         "/dev/shm/nerdob2.sqlite3"
#define DB_DISK        "database.sqlite3"


#define DEVICE          "/dev/ttyUSB0"
#define BAUDRATE        10400   // for my seat arosa, vw polo needs 9600
#define WRITE_DELAY     0    // milliseconds delay before serial writes, not needed anymore. (was 5700)
#define INIT_DELAY      200000  // nanosec to wait to emulate 5baud

/* constants for calculating consumption
 * out of injection time, rpm and speed
 *
 * consumption per hour =
 *    MULTIPLIER * rpm * injection time
 * 
 * you need to adjust this multiplier to (roughly) match your car
 * some help in finding it out might be this (after recording some data
 * and refuling the car, knowing exactly what it actually consumed):
 * 
 * select SUM((YOUR_MULTIPLIER_HERE * rpm * injection_time) / speed * 100 * speed) / SUM(speed) from engine_data where speed > 0;
 */
#define MULTIPLIER      0.000215500  // for my 1.0l seat arosa 2004
