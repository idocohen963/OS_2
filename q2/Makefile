CC = gcc
CFLAGS = -Wall -std=c99 -D_POSIX_C_SOURCE=200112L

all: molecule_supplier molecule_requester atom_supplier

molecule_supplier: molecule_supplier.c
	$(CC) $(CFLAGS) -o molecule_supplier molecule_supplier.c

molecule_requester: molecule_requester.c
	$(CC) $(CFLAGS) -o molecule_requester molecule_requester.c

atom_supplier: atom_supplier.c
	$(CC) $(CFLAGS) -o atom_supplier atom_supplier.c

clean:
	rm -f molecule_supplier molecule_requester atom_supplier

.PHONY: all clean