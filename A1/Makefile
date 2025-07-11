# Makefile for building libvcparser.so
# Author: [Your Name], Student ID: [Your Student ID]

CC = gcc
CFLAGS = -Wall -fPIC -Iinclude
LDFLAGS = -shared
SRC = src/VCParser.c src/VCHelpers.c src/LinkedListAPI.c
OBJ = $(SRC:.c=.o)
TARGET = bin/libvcparser.so

.PHONY: all clean parser

all: parser

parser: $(OBJ)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
