CC=gcc
CC_OPTIONS_CORE=-Wall -O3 # -lxxx breaks serial connection :/
CC_OPTIONS_SQLITE=-Wall -O3 -lsqlite3
CC_OPTIONS=-Wall -O3


nerdobd2 : core db_server

# core
core : core.o kw1281.o db_client.o
	$(CC) $(CC_OPTIONS_CORE) -o nerdobd2_core core/core.o core/kw1281.o core/db_client.o

core.o : core/core.c
	$(CC) $(CC_OPTIONS_CORE) -c core/core.c -o core/core.o
	
kw1281.o : core/kw1281.c
	$(CC) $(CC_OPTIONS_CORE) -c core/kw1281.c -o core/kw1281.o

db_client.o : core/db_client.c
	$(CC) $(CC_OPTIONS_CORE) -c core/db_client.c -o core/db_client.o


# sqlite	
db_server : db_server.o sqlite.o tcp.o
	$(CC) $(CC_OPTIONS_SQLITE) -o nerdobd2_dbserver sqlite/db_server.o  sqlite/sqlite.o common/tcp.o

db_server.o : sqlite/db_server.c
	$(CC) $(CC_OPTIONS_SQLITE) -c sqlite/db_server.c -o sqlite/db_server.o
	
sqlite.o : sqlite/sqlite.c
	$(CC) $(CC_OPTIONS_SQLITE) -c sqlite/sqlite.c -o sqlite/sqlite.o


# tcp helpers
tcp.o : common/tcp.c
	$(CC) $(CC_OPTIONS) -c common/tcp.c -o common/tcp.o

# json helpers
json.o : json/json.c
	$(CC) $(CC_OPTIONS) -c json/json.c -o json/json.o


# cleaning	
clean :
	rm -f core/*.o sqlite/*.o nerdobd2_*

