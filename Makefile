CC=gcc
CC_OPTIONS=-Wall # -ggdb


nerdobd2 : main.o kw1281.o
	$(CC) $(CC_OPTIONS) -o nerdobd2 main.o kw1281.o

main.o : main.c
	$(CC) $(CC_OPTIONS) -c main.c
	
kw1281.o : kw1281.c
	$(CC) $(CC_OPTIONS) -c kw1281.c

clean :
	rm -f *.o nerdobd2
