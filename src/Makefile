
CC=gcc-5

LD=$(CC)
AR=ar

DEBUG=-g3
OPT=-O3
CFLAGS=$(OPT) -Wall $(DEBUG) $(INCPATHS) -std=c11
LDFLAGS=$(OPT) $(DEBUG) $(LIBPATHS) -L.

LIBSOURCES=builtin.c scanner.c reader.c printer.c eval.c env.c util.c
LIBOBJS=$(LIBSOURCES:%.c=%.o)

TARGET=rudel

.PHONY:	all clean

.SUFFIXES: .c .o

all: $(TARGET)

.deps: *.c *.h
	$(CC) $(CFLAGS) -MM *.c > .deps

$(TARGET): %: main.o librudel.a
	$(LD) $^ -o $@ $(LDFLAGS)

librudel.a: $(LIBOBJS)
	$(AR) rcs $@ $^

test:	$(TARGET)
	../scr/runtest.py ../tests/tests.rd ./$(TARGET)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o $(TARGET) librudel.a .deps

-include .deps

