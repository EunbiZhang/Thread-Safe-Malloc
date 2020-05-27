CC=gcc
CFLAGS=-O3 -fPIC -Wall -Werror
DEPS=my_malloc.h my_malloc.c

all: lib
lib: libmymalloc.so

libmymalloc.so: my_malloc.o
	$(CC) $(CFLAGS) -shared -o $@ $< -g

%.o: %.c my_malloc.h my_malloc.c
	$(CC) $(CFLAGS) -c -o $@ $< -g

clean:
	rm -f *~ *.o *.so

clobber:
	rm -f *~ *.o
