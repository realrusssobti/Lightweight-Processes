CC 	= gcc

CFLAGS  = -Wall -g -arch x86_64 -I ../include

LD 	= gcc

LDFLAGS  = -Wall -g -L ../lib64

PROGS	= snakes nums hungry

SNAKEOBJS  = randomsnakes.o util.o

SNAKELIBS = lib64/libPLN.a lib64/libsnakes.a -lncurses

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

snakes: randomsnakes.o util.o lib64/libPLN.a lib64/libsnakes.a
	$(LD) $(LDFLAGS) -o snakes randomsnakes.o util.o $(SNAKELIBS)

hungry: hungrysnakes.o  util.o lib64/libPLN.a lib64/libsnakes.a
	$(LD) $(LDFLAGS) -o hungry hungrysnakes.o util.o $(SNAKELIBS)

nums: numbermain.o  util.o lib64/libPLN.a
	$(LD) $(LDFLAGS) -o nums numbersmain.o ../lib64/libPLN.a

hungrysnakes.o: demos/hungrysnakes.c include/lwp.h include/snakes.h
	$(CC) $(CFLAGS) -c demos/hungrysnakes.c

randomsnakes.o: demos/randomsnakes.c ./include/lwp.h include/snakes.h
	$(CC) $(CFLAGS) -c demos/randomsnakes.c

numbermain.o: demos/numbersmain.c include/lwp.h
	$(CC) $(CFLAGS) -c demos/numbersmain.c

util.o: demos/util.c include/lwp.h include/util.h include/snakes.h
	$(CC) $(CFLAGS) -c demos/util.c

rs: snakes
	(export LD_LIBRARY_PATH=../lib64; ./snakes)

hs: hungry
	(export LD_LIBRARY_PATH=../lib64; ./hungry)

ns: nums
	(export LD_LIBRARY_PATH=../lib64; ./nums)

rr.o: src/rr.c include/lwp.h
	$(CC) $(CFLAGS) -o rr.o -c rr.c

libPLN.so: lib64/libPLN.a
		$(CC) -shared -o $@ $<
libsnakes.so: lib64/libsnakes.a
		$(CC) -shared -o $@ $<