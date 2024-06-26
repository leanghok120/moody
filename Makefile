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

install: all
	cp moody /usr/bin/
	cp ./sxhkd/ ~/.config/ -r
	chmod 755 /usr/bin/moody
