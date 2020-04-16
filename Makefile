CC=gcc
CFLAGS=-Wall -g

SOURCES= dkt.c

OBJECTS= dkt.o

dkt: dkt.o
	    $(CC) -g -o $@ $(OBJECTS)

main.o: main.c
		$(CC) -c $(CFLAGS) dkt.c

clean::
	rm -f *.o core a.out dkt *~

depend::
	makedepend $(SOURCES)
