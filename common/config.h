#define DB_HOST	        "127.0.0.1"
#define DB_PORT         4588
#define HTTPD_PORT	8080
#define DOCROOT		"httpd"

//#define DEBUG_SERIAL
//#define DEBUG_SQLITE
#define DEBUG_AJAX

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
 *    60 (minutes) * 4 (zylinders) * 
 *    MULTIPLIER * rpm * (injection time - INJ_SUBTRACT)
 */
#define MULTIPLIER      0.00000089  // for my 1.0l seat arosa 2004
#define INJ_SUBTRACT    0.1

