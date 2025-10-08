all: build

build:
	gcc -o moody moody.c -lX11

test:
	Xephyr -ac -screen 1200x800 :1 &
	sleep 0.5
	DISPLAY=:1 ./moody &
	sleep 0.3
	DISPLAY=:1 st
