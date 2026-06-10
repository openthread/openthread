CC?=gcc
CFLAGS=-I ../../../../include -O2 -Wall

all: test_all

%.o: ../%.c
	$(CC) -c $(CFLAGS) $< -o $@

test_all.o: test_all.c
	$(CC) -c $(CFLAGS) test_all.c -o $@

test_all: test_all.o cbuf.o lbuf.o bitmap.o
	$(CC) test_all.o cbuf.o lbuf.o bitmap.o -o test_all

clean:
	rm -f *.o test_all
