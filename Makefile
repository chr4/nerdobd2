CC=gcc
CC_OPTIONS=-Wall -lpthread -lrrd -lsqlite3 # -ggdb


nerdobd2 : main.o kw1281.o rrdtool.o ajax.o sqlite.o
	$(CC) $(CC_OPTIONS) -o nerdobd2 main.o kw1281.o rrdtool.o ajax.o sqlite.o

main.o : main.c
	$(CC) $(CC_OPTIONS) -c main.c
	
kw1281.o : kw1281.c
	$(CC) $(CC_OPTIONS) -c kw1281.c

rrdtool.o : rrdtool.c
	$(CC) $(CC_OPTIONS) -c rrdtool.c

ajax.o : ajax.c
	$(CC) $(CC_OPTIONS) -c ajax.c

sqlite.o : sqlite.c
	$(CC) $(CC_OPTIONS) -c sqlite.c

clean :
	rm -f *.o nerdobd2

proper :
	rm -f *.png *.rrd *.sqlite3
