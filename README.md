# Mark-Sweep Lab

An educational mark-and-sweep garbage collector written in portable C11.
It manages integers, strings, and pairs, supports cyclic object graphs, and
ships with a reusable library API, tests, a stress-test CLI, and heap graph
export.

This project is developed from Robert Nystrom's
[mark-sweep example](https://github.com/munificent/mark-sweep), originally
published with
[Baby's First Garbage Collector](https://journal.stuffwithstuff.com/2013/12/08/babys-first-garbage-collector/).
The original MIT copyright notice is retained in [`COPYRIGHT`](COPYRIGHT).

## Features

- Portable C11 library with an opaque VM and object API.
- Dynamically growing root stack.
- Iterative marking that handles deep and cyclic graphs without C recursion.
- Managed integer, string, and pair objects.
- Scoped roots for temporary allocations.
- Weak references that clear automatically after collection.
- Configurable collection threshold and lifetime statistics.
- Graphviz DOT export for inspecting the heap.
- Strict warning settings and tests on Windows and Linux.

## Build

### CMake

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The build creates:

- `mark_sweep`: the collector library.
- `mark_sweep_demo`: the demo and stress-test CLI.
- `mark_sweep_tests`: the test suite.

### Make

On a Unix-like system:

```sh
make
make test
```

## Run the demo

```sh
./build/mark_sweep_demo --iterations 25000 --threshold 64 --dot heap.dot
```

On a multi-configuration Windows build, the executable is normally under
`build/Release/`.

Options:

```text
--iterations N  Allocate N temporary object graphs
--threshold N   Set the minimum automatic collection threshold
--dot FILE      Export the surviving heap as Graphviz DOT
--help          Show CLI help
```

Render an exported graph with Graphviz:

```sh
dot -Tsvg heap.dot -o heap.svg
```

## Library example

```c
#include "gc.h"

GcVm* vm = gc_vm_new();
GcRootScope scope = gc_scope_begin(vm);

gc_push_int(vm, 1);
gc_push_string(vm, "two");
GcObject* pair = gc_push_pair(vm);

gc_object_print(pair, stdout); /* (1, "two") */
gc_scope_end(vm, scope);
gc_collect(vm);
gc_vm_free(vm);
```

Objects are protected from collection only while reachable from the VM root
stack. Raw `GcObject*` pointers are non-owning and become invalid after their
object is collected. Use `GcWeakRef` when a reference must safely observe an
object without keeping it alive.

## Project scope

This collector is intentionally small and educational. It is not a drop-in
replacement for a production memory manager: it is single-threaded, uses a
stop-the-world collection, and does not compact memory. See
[`docs/design.md`](docs/design.md) for the algorithm and extension points.

## License

MIT. See [`COPYRIGHT`](COPYRIGHT).
