# Simple Makefile for emcli project

CC = gcc
CFLAGS ?= -Wall -Wextra -Wpedantic -std=c99
LDFLAGS ?=
LDLIBS ?=
RM ?= rm -f
SOURCES = main.c cli.c getset_protocol.c jsmn.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = emcli

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)
