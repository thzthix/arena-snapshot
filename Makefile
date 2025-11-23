CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -Iinclude
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
TARGET = test

OBJS = $(BUILD_DIR)/arena.o $(BUILD_DIR)/test.o

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(BUILD_DIR)/arena.o: $(SRC_DIR)/arena.c $(INC_DIR)/arena.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/arena.c -o $(BUILD_DIR)/arena.o

$(BUILD_DIR)/test.o: $(SRC_DIR)/test.c $(INC_DIR)/arena.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/test.c -o $(BUILD_DIR)/test.o

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
