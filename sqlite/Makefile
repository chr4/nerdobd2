CC=gcc
CC_OPTIONS=-Wall -lsqlite3

nerdobd2 : main.o
	$(CC) $(CC_OPTIONS) -o sqlite main.o

main.o : main.c
	$(CC) $(CC_OPTIONS) -c main.c
	
clean :
	rm -f *.o sqlite
