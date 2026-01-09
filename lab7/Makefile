CC=gcc
CFLAGS=-Wall -O2

all: sender receiver

sender: sender.c
        $(CC) $(CFLAGS) sender.c -o sender

receiver: receiver.c
        $(CC) $(CFLAGS) receiver.c -o receiver

clean:
        rm -f sender receiver

fclean: clean
        rm -f /tmp/sender.lock
