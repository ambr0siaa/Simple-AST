CC = gcc
SRC = main.c parser.c lexer.c sv.c
CFLAGS = -Wall -Wextra
EXE = main

all:
	$(CC) $(SRC) $(CFLAGS) -o $(EXE)