CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror -g
LDFLAGS ?= -pthread -lrt
SRCS = aesdsocket.c
EXEC = aesdsocket

default : $(EXEC)
all : $(EXEC)

$(EXEC): $(SRCS)
	$(CC) $(SRCS) $(CFLAGS) $(LDFLAGS) -o $(EXEC)

clean:
	-rm -rf *.o $(EXEC)
