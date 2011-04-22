CC=gcc
CC_OPTIONS_CORE=-Wall # -lxxx breaks serial connection :/
CC_OPTIONS_SQLITE=-Wall -lsqlite3 -ljson
CC_OPTIONS=-Wall


nerdobd2 : core sqlite

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
sqlite : db_server.o sqlite.o json.o ajax.o
	$(CC) $(CC_OPTIONS_SQLITE) -o nerdobd2_sqlite sqlite/db_server.o sqlite/sqlite.o ajax/ajax.o json/json.o

db_server.o : sqlite/db_server.c
	$(CC) $(CC_OPTIONS_SQLITE) -c sqlite/db_server.c -o sqlite/db_server.o
	
sqlite.o : sqlite/sqlite.c
	$(CC) $(CC_OPTIONS_SQLITE) -c sqlite/sqlite.c -o sqlite/sqlite.o
	
ajax.o : ajax/ajax.c
	$(CC) $(CC_OPTIONS_SQLITE) -c ajax/ajax.c -o ajax/ajax.o


# json helpers
json.o : json/json.c
	$(CC) $(CC_OPTIONS) -c json/json.c -o json/json.o


# cleaning	
clean :
	rm -f core/*.o sqlite/*.o nerdobd2_*

