CC = gcc

CFLAGS = -Wall -Wextra -O2

TARGET = main

$(TARGET): main.o
	$(CC) $(CFLAGS) -o $(TARGET) main.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

run: main
	./main

clean:
	rm -f $(TARGET) main.o
