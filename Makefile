all: build

build:
	gcc -o moody moody.c -lX11

test:
	xinit .xinitrc
