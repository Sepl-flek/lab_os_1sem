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
<<<<<<< HEAD
        rm -f /tmp/sender.lock
=======
	rm -f /tmp/sender.lock

>>>>>>> 228e24c18da90ecd83508bb01a2f457ea4d36831
