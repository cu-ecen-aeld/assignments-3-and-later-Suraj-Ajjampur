COMPILER=$(CROSS_COMPILE)gcc
all: writer

writer: writer.o
	$(COMPILER) -o writer writer.o
	chmod +x writer

writer.o: writer.c
	$(COMPILER) -c writer.c

clean:
	rm -f  writer.o writer
