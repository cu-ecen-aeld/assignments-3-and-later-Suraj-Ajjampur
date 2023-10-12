# Cross compiler
CC=$(CROSS_COMPILE)gcc

# Compiler Flags
CFLAGS += -Wall -Werror -O0 -g

# Executable
.PHONY: all
all: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.o
	chmod +x aesdsocket

aesdsocket1.o: aesdsocket.c
	$(CC) $(CFLAGS) -c aesdsocket.c

.PHONY: clean
clean:
	rm -f  aesdsocket.o aesdsocket
