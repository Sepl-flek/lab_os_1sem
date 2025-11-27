CC=gcc
CFLAGS=-Wall -Wextra -O2

all: pipe_fork writer reader

pipe_fork: pipe_fork.c
	$(CC) $(CFLAGS) pipe_fork.c -o pipe_fork

writer: writer.c
	$(CC) $(CFLAGS) writer.c -o writer

reader: reader.c
	$(CC) $(CFLAGS) reader.c -o reader

clean:
	rm -f pipe_fork writer reader myfifo
