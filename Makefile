CC = gcc
TARGET = test
CFLAGS = -Wall -Wextra

TARGET_PATH = ./tests/
SRC_PATH = ./src/
SRC = $(wildcard $(SRC_PATH)*.c)

$(TARGET): $(SRC)
	$(CC) $(TARGET_PATH)$(TARGET).c $(SRC) $(CFLAGS) -o $(TARGET)