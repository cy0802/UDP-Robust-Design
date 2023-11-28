CC = gcc
CFLAGS = -static

all: udpechosrv udpping

udpechosrv: udpechosrv.c
	$(CC) $(CFLAGS) -o udpechosrv udpechosrv.c

udpping: udpping.c
	$(CC) $(CFLAGS) -o udpping udpping.c

.PHONY: clean

clean:
	rm -f udpechosrv udpping
