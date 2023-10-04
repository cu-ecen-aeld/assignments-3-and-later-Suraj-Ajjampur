# Cross compiler
CC=$(CROSS_COMPILE)gcc

# Compiler Flags
CFLAGS += -Wall -Werror

# Executable
all: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.o
	chmod +x aesdsocket

aesdsocket.o: aesdsocket.c
	$(CC) $(CFLAGS) -c aesdsocket.c

clean:
	rm -f  aesdsocket.o aesdsocket
