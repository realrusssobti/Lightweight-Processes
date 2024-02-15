CC = gcc
CFLAGS = -Wall -g -I ./include

all:
	make liblwp.so

liblwp.so: magic64.o lwp.o rr.o
	$(CC) $(CFLAGS) -shared -o $@ $^

magic64.o: magic64.S
	$(CC) -o $@ -c $<

lwp.o: lwp.c
	$(CC) $(CFLAGS) -o $@ -c $<

rr.o: demos/rr.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.o
