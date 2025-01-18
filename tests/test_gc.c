#include "gc.h"

#include <stdio.h>
#include <stdlib.h>

static int failures = 0;

static void check(bool condition, const char* message) {
  if (!condition) {
    fprintf(stderr, "FAIL: %s\n", message);
    failures++;
  }
}

static void test_roots_are_preserved(void) {
  GcVm* vm = gc_vm_new();
  check(vm != NULL, "VM allocation succeeds");
  check(gc_push_int(vm, 1), "first integer allocation succeeds");
  check(gc_push_int(vm, 2), "second integer allocation succeeds");
  check(gc_collect(vm) == 0, "rooted objects are not collected");
  check(gc_object_count(vm) == 2, "two rooted objects remain");
  gc_vm_free(vm);
}

static void test_unreachable_objects_are_collected(void) {
  GcVm* vm = gc_vm_new();
  gc_push_int(vm, 1);
  gc_push_int(vm, 2);
  gc_pop(vm);
  gc_pop(vm);
  check(gc_collect(vm) == 2, "two unreachable objects are collected");
  check(gc_object_count(vm) == 0, "heap is empty");
  gc_vm_free(vm);
}

static void test_nested_objects_are_reached(void) {
  GcVm* vm = gc_vm_new();
  gc_push_int(vm, 1);
  gc_push_int(vm, 2);
  gc_push_pair(vm);
  gc_push_int(vm, 3);
  gc_push_int(vm, 4);
  gc_push_pair(vm);
  gc_push_pair(vm);
  check(gc_collect(vm) == 0, "nested graph stays reachable");
  check(gc_object_count(vm) == 7, "all seven nested objects remain");
  gc_vm_free(vm);
}

static void test_cycles_are_supported(void) {
  GcVm* vm = gc_vm_new();
  GcObject* a;
  GcObject* b;

  gc_push_int(vm, 1);
  gc_push_int(vm, 2);
  a = gc_push_pair(vm);
  gc_push_int(vm, 3);
  gc_push_int(vm, 4);
  b = gc_push_pair(vm);

  gc_pair_set_tail(a, b);
  gc_pair_set_tail(b, a);

  check(gc_collect(vm) == 2, "unreferenced pair members are collected");
  check(gc_object_count(vm) == 4, "cycle and reachable members remain");
  gc_vm_free(vm);
}

int main(void) {
  test_roots_are_preserved();
  test_unreachable_objects_are_collected();
  test_nested_objects_are_reached();
  test_cycles_are_supported();

  if (failures != 0) {
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return EXIT_FAILURE;
  }

  puts("All garbage collector tests passed.");
  return EXIT_SUCCESS;
}
