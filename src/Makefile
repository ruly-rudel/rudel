
CC=gcc-5

LD=$(CC)
AR=ar

#DEBUG_OPT=-g3 -O0 -DEBUG -DDONT_ABORT_EVAL_ON_ERROR
DEBUG_OPT=-g3 -O0 -DSTR_EASY_CHECK -DDEBUG -DDEBUG_CMD
PROF_OPT=-O3 -pg -DSTR_EASY_CHECK -DNDEBUG
FAST_OPT=-O3 -DSTR_EASY_CHECK -DNDEBUG
OPT=-O3 -DSTR_EASY_CHECK -DDEBUG_CMD
ARCH=-m32
CFLAGS=$(ARCH) -Wall $(INCPATHS)
LDFLAGS=$(ARCH) $(LIBPATHS) -L.

LIBSOURCES=builtin.c scanner.c reader.c printer.c eval.c env.c core.c resolv.c vm.c misc.c ../linenoise/linenoise.c
LIBOBJS=$(LIBSOURCES:%.c=%.o)

TARGET=rudel

.PHONY:	all clean debug prof

.SUFFIXES: .c .o

all: $(TARGET)

debug:	OPT=$(DEBUG_OPT)
debug:	all

prof:	OPT=$(PROF_OPT)
prof:	all

fast:	OPT=$(FAST_OPT)
fast:	all

.deps: *.c *.h
	$(CC) $(CFLAGS) -MM *.c > .deps

$(TARGET): %: main.o librudel.a
	$(LD) $^ -o $@ $(OPT) $(LDFLAGS)

librudel.a: $(LIBOBJS)
	$(AR) rcs $@ $^

test:	$(TARGET)
	../scr/runtest.py ../tests/tests.rud ./$(TARGET)

.c.o:
	$(CC) $(OPT) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o ../linenoise/*.o $(TARGET) librudel.a .deps tags ../tags gmon.out

-include .deps

