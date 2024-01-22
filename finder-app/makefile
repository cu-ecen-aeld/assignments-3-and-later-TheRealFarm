all: writer

CC = gcc
CROSS_COMPILE !=

writer: writer.c
	$(CROSS_COMPILE)$(CC) -o writer writer.c

.PHONY: clean
clean:
	rm -f writer *.o
