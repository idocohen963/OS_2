CC = gcc
CFLAGS = -Wall -std=c99 -D_POSIX_C_SOURCE=200112L

all: atom_supplier molecule_requester drinks_bar

atom_supplier: atom_supplier.c
	$(CC) $(CFLAGS) -o atom_supplier atom_supplier.c

drinks_bar : drinks_bar.c
	$(CC) $(CFLAGS) -o drinks_bar drinks_bar.c

molecule_requester: molecule_requester.c
	$(CC) $(CFLAGS) -o molecule_requester molecule_requester.c

clean:
	rm -f drinks_bar molecule_requester atom_supplier

.PHONY: all clean