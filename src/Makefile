
CC=gcc-5

LD=$(CC)
AR=ar

#DEBUG_OPT=-DNOINIT -DDUMP_ALLOC_ADDR

COMOPT=-DUSE_UTF8 -DSTR_EASY_CHECK -DDEBUG_CMD
NOOPTIMIZE=-O0 -DNO_INLINE
OPTIMIZE=-O3

#DEBUG_OPT=-g3 $(NOOPTIMIZE) -DPRINT_STACK_USAGE -DTRACE_VM -DTRACE_GC -DDEBUG_GC
DEBUG_OPT=-g3 $(NOOPTIMIZE) -DPRINT_STACK_USAGE -DTRACE_VM -DTRACE_GC
PROF_OPT=-pg $(OPTIMIZE) -DNDEBUG -DPRINT_STACK_USAGE
RELEASE_OPT=$(OPTIMIZE) -DNDEBUG
GC_OPT=$(OPTIMIZE) -DDEBUG_GC -DCHECK_GC_SANITY
COV_OPT=-coverage $(OPTIMIZE) -DNDEBUG
TEST_OPT=$(OPTIMIZE)

ARCH=-m32
CFLAGS=$(ARCH) $(COMOPT) -Wall $(INCPATHS)
LDFLAGS=$(ARCH) $(LIBPATHS) -L.

LIBSOURCES=builtin.c allocator.c scanner.c reader.c printer.c env.c vm.c compile_vm.c misc.c ../linenoise/linenoise.c ../linenoise/stringbuf.c ../linenoise/utf8.c
LIBOBJS=$(LIBSOURCES:%.c=%.o)

TARGET=rudel

.PHONY:	all clean debug prof

.SUFFIXES: .c .o

release:	OPT=$(RELEASE_OPT)
release:	all

debug:	OPT=$(DEBUG_OPT)
debug:	all

prof:	OPT=$(PROF_OPT)
prof:	all

gc:	OPT=$(GC_OPT)
gc:	all

cov:	OPT=$(COV_OPT)
cov:	$(TARGET)
	./rudel ../tests/cov.rud
	gcov *.gcda

alloc:	OPT=$(DEBUG_OPT)
alloc:	OPT=$(DEBUG_OPT)

all:
all: $(TARGET)

.deps: *.c *.h
	$(CC) $(CFLAGS) -MM *.c > .deps

$(TARGET): %: main.o librudel.a
	$(LD) $^ -o $@ $(OPT) $(LDFLAGS)

librudel.a: $(LIBOBJS)
	$(AR) rcs $@ $^

test:	OPT=$(TEST_OPT)
test:	$(TARGET)
	../scr/runtest.py --test-timeout 120 ../tests/tests.rud ./$(TARGET)

alloc: test_allocator.o librudel.a
	$(LD) $^ -lcunit -o $@ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $(OPT) -c $< -o $@

clean:
	rm -rf *.o ../linenoise/*.o $(TARGET) librudel.a .deps tags ../tags ../linenoise/tags alloc gmon.out *.gcno *.gcda *.gcov ../linenoise/*.gcda ../linenoise/*.gcno

-include .deps

