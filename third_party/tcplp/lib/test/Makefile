CC=clang
CFLAGS=-I ../../../../include -O0 -Wall
LDFLAGS=

ifeq ($(COVERAGE),1)
CFLAGS+=--coverage
LDFLAGS+=--coverage
endif

all: test_all

%.o: ../%.c
	clang -c $(CFLAGS) $< -o $@

test_all.o: test_all.c
	clang -c $(CFLAGS) test_all.c -o $@

test_all: test_all.o cbuf.o lbuf.o bitmap.o
	clang $(LDFLAGS) test_all.o cbuf.o lbuf.o bitmap.o -o test_all

clean:
	rm -f *.o test_all
