CC = gcc
CFLAGS = -Wall
LDFLAGS = -lX11

TARGET = moody

SRC = moody.c

all:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $(TARGET)

build:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -rf $(TARGET)

run:
	./preview.sh

install:
	cp moody /usr/bin/
	chmod 755 /usr/bin/moody
