# Makefile

EXE=server

CC = gcc
CFLAGS = -pthread -Wall -std=gnu99

.PHONY: all

all: $(EXE)

$(EXE): $(EXE).c
	$(CC) $(CFLAGS) -o $(EXE) skiplist.c parser.c queue.c server.c
