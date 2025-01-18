CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
CPPFLAGS ?= -Iinclude

.PHONY: all clean test run

all: demo test_gc

demo: src/gc.c examples/demo.c include/gc.h
	$(CC) $(CPPFLAGS) $(CFLAGS) src/gc.c examples/demo.c -o $@

test_gc: src/gc.c tests/test_gc.c include/gc.h
	$(CC) $(CPPFLAGS) $(CFLAGS) src/gc.c tests/test_gc.c -o $@

test: test_gc
	./test_gc

run: demo
	./demo

clean:
	rm -f demo test_gc demo.exe test_gc.exe *.obj

