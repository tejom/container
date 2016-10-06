CC=gcc

build: main.o
	$(CC) -o main main.o
