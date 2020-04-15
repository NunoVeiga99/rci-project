CC=gcc
CFLAGS=-Wall -g

SOURCES= main.c

OBJECTS= main.o

dkt: main.o
	    $(CC) -g -o $@ $(OBJECTS)

main.o: main.c
		$(CC) -c $(CFLAGS) main.c

clean::
	rm -f *.o core a.out dkt *~

depend::
	makedepend $(SOURCES)
