all: build

build:
	gcc -o cmdUtil sendUdp.c cmdUtil.c

install: build
	cp -f cmdUtil ../../bin

clean:
	rm -f cmdUtil