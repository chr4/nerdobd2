## nerdobd2

Wanna hack your car?

This tool reads out the data of your engine control unit (ECU) and writes them to a postgresql or sqlite3 database. Additionally it calculates the current consumption (using the engines RPM, the current speed and the injection time).

Additionally, it provides a small AJAX http server that serves the data live to a web interface that you can view in your browser.

it runs on linux



### setting up your obd2 device

if the system suspends and then resumes, the kernel tries to newly assign /dev/ttyUSB0 to the USB to serial adapter. this fails, because we still have ttyUSB0 open, so it dynamically creates ttyUSB1 instead, so we cannot reopen ttyUSB0. As a workaround (so you don't have to replug your cable), I setup this udev rule in /etc/udev/rules.d/70-persisent-usb-serial.rule

    SUBSYSTEMS=="usb", ATTRS{serial}=="A600bj0A", KERNEL=="ttyUSB*", SYMLINK+="obd2"

assigning the USB to serial adapter the fixed symlink /dev/obd2.
find out your serial by using lsusb -v.


### downloading

    git clone git://github.com/chr4/nerdobd2.git
    cd nerdobd2


### configuring

    vi config.h

make sure to set at least

    #define DEVICE "/dev/obd2"
    #define BAUDRATE 10400

and specify your database

    #define DB_POSTGRES "user=nerdobd2 dbname=nerdobd2"

or

    #define DB_SQLITE "database.sqlite3"


then set the constants for calculating consumption using injection time, rpm and speed

    consumption per hour = MULTIPLIER * rpm * injection time

you need to adjust this multiplier to (roughly) match your car.
This SQL statement might help in finding it (after recording some data and refuling the car, knowing exactly what it actually consumed):

    SELECT SUM((YOUR_MULTIPLIER_HERE * rpm * injection_time) / speed * 100 * speed) / SUM(speed) FROM data WHERE speed > 0;

then set the appropriate multiplier

    #define MULTIPLIER 0.000213405


### setting up gps

if you have a gps device, simply run gpsd. nerdobd2 tries to connect to gpsd automatically.


### compiling

    make


### starting it up

    ./nerdobd2

then point your browser to either

the map template (you need to download openstreetmap tiles into html/tiles before) [http://localhost:8080/map.html](http://localhost:8080/map.html)

the graphs template [http://localhost:8080/graphs.html](http://localhost:8080/graphs.html)
