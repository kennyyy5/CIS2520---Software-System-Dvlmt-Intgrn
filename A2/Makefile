# Makefile for building libvcparser.so
# Author: Kenny Adenuga, Student ID: 1304431

CC = gcc
CFLAGS = -Wall -fPIC -Iinclude
LDFLAGS = -shared
SRC = src/VCParser.c src/VCHelpers.c src/VCAssign2.c src/LinkedListAPI.c 
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
