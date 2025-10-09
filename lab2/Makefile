CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Wpedantic -O2

all: myls

myls: myls.c
	$(CC) $(CFLAGS) -o myls myls.c

clean:
	rm -f myls

.PHONY: all clean


