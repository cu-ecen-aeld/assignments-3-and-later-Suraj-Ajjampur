# Is compiled by the “all” and “default” target of a Makefile in the “server” directory and 
# supports cross compilation, placing the executable file in the “server” directory and named aesdsocket.

CC=$(CROSS_COMPILE)gcc
all: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) -o aesdsocket aesdsocket.o
	chmod +x aesdsocket

writer.o: aesdsocket.c
	$(CC) -c aesdsocket.c

clean:
	rm -f  aesdsocket.o aesdsocket
	