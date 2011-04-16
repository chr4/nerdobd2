CC=gcc
CC_OPTIONS=-Wall 
# -lsqlite3 breaks serial communication :/

core : main.o kw1281.o socket.o
	$(CC) $(CC_OPTIONS) -o core main.o kw1281.o socket.o

main.o : main.c
	$(CC) $(CC_OPTIONS) -c main.c
	
kw1281.o : kw1281.c
	$(CC) $(CC_OPTIONS) -c kw1281.c

socket.o : socket.c
	$(CC) $(CC_OPTIONS) -c socket.c
	
clean :
	rm -f *.o core
