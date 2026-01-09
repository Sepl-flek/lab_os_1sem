CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2

TARGET = pr8
SRC = pr8.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean

