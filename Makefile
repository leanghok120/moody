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
	rm -rf /usr/bin/$(TARGET)

run:
	./preview.sh

install:
	cp ./$(TARGET) /usr/bin/
	cp ./autostart.sh /usr/bin/
	cp ./moody.desktop /usr/share/xsessions/
	cp ./polybar/ ~/.config/ -r
	chmod 755 /usr/bin/$(TARGET)
	chmod 755 /usr/bin/autostart.sh
