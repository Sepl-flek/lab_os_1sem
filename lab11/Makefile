CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGET = lab11

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

clean:
	rm -f $(TARGET)
