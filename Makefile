SRCS=soil-watcher.c evgpio.c 
CFLAGS=-mcpu=arm9
CC=gcc
INSTALL=/usr/local/bin/

all: soil-watcher

soil-watcher:
	$(CC) -o soil-watcher $(SRCS) $(CFLAGS)

install: all
	install -m 4755 soil-watcher $(INSTALL)

clean:
	-rm -f soil-watcher
	-rm -f $(INSTALL)soil-watcher
