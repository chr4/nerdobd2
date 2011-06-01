#define HTTPD_PORT	   8080
#define DOCROOT		   "html"

//#define DEBUG_SERIAL
//#define DEBUG_SQLITE
//#define DEBUG_AJAX
//#define TEST

/* writing to database is too slow on my eeepc,
 * thus provoking "resource temporarily not available" errors
 * putting database in /dev/shm, and syncing it from time to time
 * should help.
 */
#define DB_RAM         "/dev/shm/nerdob2.sqlite3"
#define DB_DISK        "database.sqlite3"

/* sync the database form ram to disk every SYNC_DELAY seconds
 * nicing the rsync process with SYNC_NICE
 */
#define SYNC_NICE      19
#define SYNC_DELAY     10

/* if the system suspends and then resumes, the kernel tries to newly
 * assign /dev/ttyUSB0 to the USB to serial adapter. this fails, because
 * we still have ttyUSB0 open, so it dynamically creates ttyUSB1 instead,
 * so we cannot reopen ttyUSB0. As a workaround (so you don't have to replug
 * your cable), I setup this udev rule in 
 * /etc/udev/rules.d/70-persisent-usb-serial.rule
 *
 * SUBSYSTEMS=="usb", ATTRS{serial}=="A600bj0A", KERNEL=="ttyUSB*", SYMLINK+="obd2"
 *
 * assigning the USB to serial adapter the fixed symlink /dev/obd2.
 * find out your serial by using lsusb -v.
 */
#define DEVICE          "/dev/obd2"

#define BAUDRATE        10400   // for my seat arosa, vw polo needs 9600
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
