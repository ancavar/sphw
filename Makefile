CC = gcc
CFLAGS = -Wall -Wextra -std=c99

TARGET = ping_pong
SRC = ping_pong.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
