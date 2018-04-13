
CC=gcc-5

LD=$(CC)
AR=ar

#DEBUG_OPT=-g3 -O0 -DEBUG -DDONT_ABORT_EVAL_ON_ERROR
#DEBUG_OPT=-g3 -O0 -DSTR_EASY_CHECK -DDEBUG_CMD -DNO_INLINE -DTRACE_VM -DTRACE_GC -DPRINT_STACK_USAGE -DNOINIT -DDUMP_ALLOC_ADDR
DEBUG_OPT=-g3 -O0 -DSTR_EASY_CHECK -DDEBUG_CMD -DNO_INLINE -DPRINT_STACK_USAGE -DTRACE_VM -DTRACE_GC
PROF_OPT=-O3 -pg -DSTR_EASY_CHECK -DNDEBUG -DPRINT_STACK_USAGE
FAST_OPT=-O3 -DSTR_EASY_CHECK -DNDEBUG
OPT=-O3 -DUSE_UTF8 -DSTR_EASY_CHECK -DDEBUG_CMD
ARCH=-m32
CFLAGS=$(ARCH) -Wall $(INCPATHS)
LDFLAGS=$(ARCH) $(LIBPATHS) -L.

LIBSOURCES=builtin.c allocator.c scanner.c reader.c printer.c env.c vm.c compile_vm.c misc.c ../linenoise/linenoise.c ../linenoise/stringbuf.c ../linenoise/utf8.c
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

alloc:	OPT=$(DEBUG_OPT)

.deps: *.c *.h
	$(CC) $(CFLAGS) -MM *.c > .deps

$(TARGET): %: main.o librudel.a
	$(LD) $^ -o $@ $(OPT) $(LDFLAGS)

librudel.a: $(LIBOBJS)
	$(AR) rcs $@ $^

test:	$(TARGET)
	../scr/runtest.py ../tests/tests.rud ./$(TARGET)

alloc: test_allocator.o librudel.a
	$(LD) $^ -lcunit -o $@ $(OPT) $(LDFLAGS)

.c.o:
	$(CC) $(OPT) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o ../linenoise/*.o $(TARGET) librudel.a .deps tags ../tags alloc gmon.out

-include .deps

