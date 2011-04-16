CC=gcc
CC_OPTIONS=-Wall -lsqlite3

sqlite : socket.o sqlite.o
	$(CC) $(CC_OPTIONS) -o sqlite socket.o sqlite.o

socket.o : socket.c
	$(CC) $(CC_OPTIONS) -c socket.c
	
sqlite.o : sqlite.c
	$(CC) $(CC_OPTIONS) -c sqlite.c
	
clean :
	rm -f *.o sqlite
