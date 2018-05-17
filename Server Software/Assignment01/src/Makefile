# Makefile

EXE=demo

CC = gcc
CFLAGS = -lpthread -Wall -std=gnu99

.PHONY: all check clean


all: $(EXE)

$(EXE): $(EXE).c
	$(CC) $(CFLAGS) -o $(EXE) $(EXE).c


OUTPUT = log

check:
	# stdbuf -o0 ./$(EXE) 5 4 2 3 60 5 | tee $(OUTPUT)
	# stdbuf -o0 ./$(EXE) 3 3 2 2 90 2 | tee $(OUTPUT)
	stdbuf -o0 ./$(EXE) 3 4 4 6 10 3 | tee $(OUTPUT)
	lua tester.lua $(OUTPUT)


clean:
	rm -f $(OUTPUT)
	rm -f $(EXE)
