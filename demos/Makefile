CC 	= gcc

CFLAGS  = -Wall -g -arch x86_64 -I ../include

LD 	= gcc

LDFLAGS  = -Wall -g -L ../lib64

PROGS	= snakes nums hungry

SNAKEOBJS  = randomsnakes.o util.o

SNAKELIBS = -lPLN -lsnakes -lncurses

HUNGRYOBJS = hungrysnakes.o util.o

NUMOBJS    = numbersmain.o

OBJS	= $(SNAKEOBJS) $(HUNGRYOBJS) $(NUMOBJS) 

EXTRACLEAN = core $(PROGS)

.PHONY: all allclean clean rs hs ns

all: 	$(PROGS)

allclean: clean
	@rm -f $(EXTRACLEAN)

clean:	
	rm -f $(OBJS) *~ TAGS

snakes: ../randomsnakes.o ../util.o ../lib64/libPLN.so ../lib64/libsnakes.so
	$(LD) $(LDFLAGS) -o snakes randomsnakes.o util.o $(SNAKELIBS)

hungry: hungrysnakes.o  ../util.o ../lib64/libPLN.so../lib64/libsnakes.so
	$(LD) $(LDFLAGS) -o hungry hungrysnakes.o util.o $(SNAKELIBS)

nums: numbersmain.o  ../util.o ../libPLN.a
	$(LD) $(LDFLAGS) -o nums numbersmain.o -lPLN

hungrysnakes.o: ../hungrysnakes.c ../lwp.h ../snakes.h
	$(CC) $(CFLAGS) -c hungrysnakes.c

randomsnakes.o: ../randomsnakes.c ../lwp.h ../snakes.h
	$(CC) $(CFLAGS) -c randomsnakes.c

numbermain.o: ../numbersmain.c lwp.h
	$(CC) $(CFLAGS) -c numbersmain.c

util.o: ../util.c ../lwp.h ../util.h ../snakes.h
	$(CC) $(CFLAGS) -c util.c

rs: snakes
	(export LD_LIBRARY_PATH=../lib64; ./snakes)

hs: ../hungry
	(export LD_LIBRARY_PATH=../lib64; ./hungry)

ns: ../nums
	(export LD_LIBRARY_PATH=../lib64; ./nums)

rr.o: ../rr.c ../lwp.h
	$(CC) $(CFLAGS) -o rr.o -c rr.c

libPLN.so: ../libPLN.a
		$(CC) -shared -o $@ $<
libsnakes.so: ../libsnakes.a
		$(CC) -shared -o $@ $<