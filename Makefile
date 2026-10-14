CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
TARGET = apc
OBJECTS = main.o apc.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

main.o: main.c apc.h
	$(CC) $(CFLAGS) -c main.c

apc.o: apc.c apc.h
	$(CC) $(CFLAGS) -c apc.c

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	./tests.sh

.PHONY: all clean test
