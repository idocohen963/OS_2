CC = gcc
CFLAGS = -Wall -std=c99 -D_POSIX_C_SOURCE=200112L

all: molecule_supplier molecule_requester

molecule_supplier: molecule_supplier.c
	$(CC) $(CFLAGS) -o molecule_supplier molecule_supplier.c

molecule_requester: molecule_requester.c
	$(CC) $(CFLAGS) -o molecule_requester molecule_requester.c

clean:
	rm -f molecule_supplier molecule_requester

.PHONY: all clean