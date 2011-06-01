CC=gcc
CC_OPTIONS=-Wall -O3 -lsqlite3 -ljson

nerdobd2 : core

# core
core : core.o kw1281.o sqlite.o httpd.o db2json.o json.o tcp.o
	$(CC) $(CC_OPTIONS) -o nerdobd2 core/core.o core/kw1281.o common/sqlite.o common/db2json.o common/httpd.o common/tcp.o common/json.o

core.o : core/core.c
	$(CC) $(CC_OPTIONS) -c core/core.c -o core/core.o
	
kw1281.o : core/kw1281.c
	$(CC) $(CC_OPTIONS) -c core/kw1281.c -o core/kw1281.o


# http server
httpd.o : common/httpd.c
	$(CC) $(CC_OPTIONS) -c common/httpd.c -o common/httpd.o
	
db2json.o : common/db2json.c
	$(CC) $(CC_OPTIONS) -c common/db2json.c -o common/db2json.o


# sqlite access
sqlite.o : common/sqlite.c
	$(CC) $(CC_OPTIONS) -c common/sqlite.c -o common/sqlite.o

# tcp helpers
tcp.o : common/tcp.c
	$(CC) $(CC_OPTIONS) -c common/tcp.c -o common/tcp.o

# json helpers
json.o : common/json.c
	$(CC) $(CC_OPTIONS) -c common/json.c -o common/json.o


# cleaning	
clean :
	rm -f core/*.o common/*.o nerdobd2_*

