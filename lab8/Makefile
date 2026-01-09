CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGET = practice8
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
