all: aesdsocket

CC = gcc
CROSS_COMPILE !=

aesdsocket: aesdsocket.c
		$(CROSS_COMPILE)$(CC) -o aesdsocket aesdsocket.c 

.PHONY: clean
clean:
	rm -f aesdsocket *.o
