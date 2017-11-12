CC=gcc
CFLAGS=-I.

prog: cost.o
	$(CC) -O0 -o prog cost.o -I.

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~