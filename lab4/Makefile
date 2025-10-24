CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
TARGET = mychmod
SOURCE = mychmod.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

.PHONY: all clean
