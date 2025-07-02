CC = clang++ 

CFLAGS = -Wall -Wextra -O2

TARGET = main

$(TARGET): main.o
	$(CC) $(CFLAGS) -o $(TARGET) main.o

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp

run: main
	./main

clean:
	rm -f $(TARGET) main.o
