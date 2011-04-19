#define UNIX_SOCKET     "../nerdobd2_socket"

//#define DEBUG_SERIAL
//#define DEBUG_SQLITE

/* writing to database is too slow on my eeepc, 
 * thus provoking "resource temporarily not available" errors
 * putting database in /dev/shm, and syncing it from time to time
 * should help.
 */
#define DB_RAM         "/dev/shm/nerdob2.sqlite3"
#define DB_DISK        "../nerdobd2.sqlite3"

#define HIGH_PRIORITY       // whether to run program with high priority (for more reliable serial communication)

#define DEVICE          "/dev/ttyUSB0"
#define BAUDRATE        10400   // for my seat arosa, vw polo needs 9600
#define WRITE_DELAY     5700    // serial dump logs 15600ms before answer arrives maybe 7800 is better?
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

