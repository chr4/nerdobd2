CC=gcc
CC_OPTIONS_CORE=-Wall # -lsqlite3 breaks serial communication :/
CC_OPTIONS=-Wall -lsqlite3 -ljson


nerdobd2 : core sqlite

# core
core : main.o kw1281.o db_client.o
	$(CC) $(CC_OPTIONS_CORE) -o nerdobd2_core core/main.o core/kw1281.o core/db_client.o

main.o : core/main.c
	$(CC) $(CC_OPTIONS_CORE) -c core/main.c -o core/main.o
	
kw1281.o : core/kw1281.c
	$(CC) $(CC_OPTIONS_CORE) -c core/kw1281.c -o core/kw1281.o

db_client.o : core/db_client.c
	$(CC) $(CC_OPTIONS_CORE) -c core/db_client.c -o core/db_client.o


# sqlite	
sqlite : socket.o sqlite.o json.o ajax.o
	$(CC) $(CC_OPTIONS) -o nerdobd2_sqlite sqlite/socket.o sqlite/sqlite.o sqlite/json.o sqlite/ajax.o

socket.o : sqlite/socket.c
	$(CC) $(CC_OPTIONS) -c sqlite/socket.c -o sqlite/socket.o
	
sqlite.o : sqlite/sqlite.c
	$(CC) $(CC_OPTIONS) -c sqlite/sqlite.c -o sqlite/sqlite.o
	
json.o : sqlite/json.c
	$(CC) $(CC_OPTIONS) -c sqlite/json.c -o sqlite/json.o
	
ajax.o : sqlite/ajax.c
	$(CC) $(CC_OPTIONS) -c sqlite/ajax.c -o sqlite/ajax.o


# cleaning	
clean :
	rm -f core/*.o sqlite/*.o nerdobd2_*

