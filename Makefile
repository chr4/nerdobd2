CC=gcc
CC_OPTIONS=-Wall -O3 -lsqlite3


nerdobd2 : core 

# core
core : core.o kw1281.o sqlite.o
	$(CC) $(CC_OPTIONS) -o nerdobd2_core core/core.o core/kw1281.o common/sqlite.o

core.o : core/core.c
	$(CC) $(CC_OPTIONS) -c core/core.c -o core/core.o
	
kw1281.o : core/kw1281.c
	$(CC) $(CC_OPTIONS) -c core/kw1281.c -o core/kw1281.o


# http server
httpd : httpd.o tcp.o
	$(CC) $(CC_OPTIONS) -o nerdobd2_httpd httpd/httpd.o common/tcp.o

httpd.o : httpd/httpd.c
	$(CC) $(CC_OPTIONS) -c httpd/httpd.c -o httpd/httpd.o
	

# sqlite access
sqlite.o : common/sqlite.c
	$(CC) $(CC_OPTIONS) -c common/sqlite.c -o common/sqlite.o

# tcp helpers
tcp.o : common/tcp.c
	$(CC) $(CC_OPTIONS) -c common/tcp.c -o common/tcp.o

# json helpers
json.o : json/json.c
	$(CC) $(CC_OPTIONS) -c json/json.c -o json/json.o


# cleaning	
clean :
	rm -f core/*.o httpd/*.o common/*.o nerdobd2_*

