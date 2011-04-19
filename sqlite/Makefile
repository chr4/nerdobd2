CC=gcc
CC_OPTIONS=-Wall -lsqlite3 -ljson

sqlite : socket.o sqlite.o json.o ajax.o
	$(CC) $(CC_OPTIONS) -o sqlite socket.o sqlite.o json.o ajax.o

socket.o : socket.c
	$(CC) $(CC_OPTIONS) -c socket.c
	
sqlite.o : sqlite.c
	$(CC) $(CC_OPTIONS) -c sqlite.c
	
json.o : json.c
	$(CC) $(CC_OPTIONS) -c json.c
	
ajax.o : ajax.c
	$(CC) $(CC_OPTIONS) -c ajax.c
	
clean :
	rm -f *.o sqlite
