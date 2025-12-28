all: moody

moody: moody.c moody.h config.h
	gcc -o moody moody.c -lX11

clean:
	rm -f /usr/local/bin/moody

install:
	mv ./moody /usr/local/bin

test:
	Xephyr -ac -screen 1200x800 :1 &
	sleep 0.5
	DISPLAY=:1 ./moody &
	sleep 0.3
	DISPLAY=:1 st
