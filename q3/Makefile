CC = gcc
CFLAGS = -Wall -std=c99 -D_POSIX_C_SOURCE=200112L

all : drinks_bar

drinks_bar : drinks_bar.c
	$(CC) $(CFLAGS) -o drinks_bar drinks_bar.c

clean:
	rm -f drinks_bar

.PHONY: all clean