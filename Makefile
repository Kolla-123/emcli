# Simple Makefile for emcli project

CC = gcc
CFLAGS = -Wall -Wextra -std=c99
SOURCES = main.c cli.c jsmn.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = emcli

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)
