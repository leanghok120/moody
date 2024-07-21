CC = gcc
CFLAGS = -Wall
LDFLAGS = -lX11

TARGET = moody

SRC = moody.c src/wm.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ) $(TARGET) /usr/bin/$(TARGET)

run:
	./preview.sh

install:
	cp ./$(TARGET) /usr/bin/
	cp ./autostart.sh /usr/bin/
	cp ./moody.desktop /usr/share/xsessions/
	chmod 755 /usr/bin/$(TARGET)
	chmod 755 /usr/bin/autostart.sh
