CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGET = pr9_1

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

clean:
	rm -f $(TARGET)
