CC = gcc
SRC = parser.c lexer.c sv.c var.c
CFLAGS = -Wall -Wextra

main:
	$(CC) main.c $(SRC) $(CFLAGS) -o main -g