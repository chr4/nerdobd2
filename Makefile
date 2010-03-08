CC=gcc
CC_OPTIONS=-Wall -lpthread -ggdb


serial : main.o kw1281.o rrdtool.o ajax.o
	$(CC) $(CC_OPTIONS) -o serial main.o kw1281.o rrdtool.o ajax.o

main.o : main.c
	$(CC) $(CC_OPTIONS) -c main.c
	
kw1281.o : kw1281.c
	$(CC) $(CC_OPTIONS) -c kw1281.c

rrdtool.o : rrdtool.c
	$(CC) $(CC_OPTIONS) -c rrdtool.c

ajax.o : ajax.c
	$(CC) $(CC_OPTIONS) -c ajax.c


clean :
	rm -f *.o serial
