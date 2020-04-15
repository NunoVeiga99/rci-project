CC=gcc
CFLAGS=-Wall -O3

SOURCES= main.c

OBJECTS= main.o

dkt: main.o
	    $(CC) -O3 -o $@ $(OBJECTS)

main.o: main.c
		$(CC) -c $(CFLAGS) main.c

clean::
	rm -f *.o core a.out dkt *~

depend::
	makedepend $(SOURCES)
