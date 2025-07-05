#include "gc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void test_root_stack_grows(void) {
  GcVm* vm = gc_vm_new();
  size_t index;

  for (index = 0; index < 4096; index++) {
    check(gc_push_int(vm, (int)index), "dynamic root push succeeds");
  }

  check(gc_stack_size(vm) == 4096, "root stack holds 4096 entries");
  check(gc_stack_capacity(vm) >= 4096, "root stack capacity grows");
  check(gc_collect(vm) == 0, "all dynamic roots remain reachable");

  for (index = 0; index < 4096; index++) {
    gc_pop(vm);
  }

  check(gc_collect(vm) == 4096, "dynamic roots are collected after pop");
  gc_vm_free(vm);
}

static void test_deep_graph_does_not_use_call_stack(void) {
  GcVm* vm = gc_vm_new();
  size_t index;

  gc_push_int(vm, 0);
  for (index = 0; index < 10000; index++) {
    check(gc_push_int(vm, (int)index), "deep graph leaf allocation succeeds");
    check(gc_push_pair(vm) != NULL, "deep graph pair allocation succeeds");
  }

  check(gc_object_count(vm) == 20001, "deep graph contains every object");
  check(gc_collect(vm) == 0, "deep graph remains reachable");
  gc_pop(vm);
  check(gc_collect(vm) == 20001, "deep graph is collected without recursion");
  gc_vm_free(vm);
}

static void test_strings_are_managed(void) {
  GcVm* vm = gc_vm_new();
  GcObject* string;

  check(gc_push_string(vm, "mark and sweep"), "string allocation succeeds");
  string = gc_pop(vm);
  check(gc_object_type(string) == GC_OBJ_STRING, "object reports string type");
  check(strcmp(gc_string_value(string), "mark and sweep") == 0,
        "managed string keeps its value");
  check(gc_collect(vm) == 1, "unreachable string is collected");
  gc_vm_free(vm);
}

int main(void) {
  test_roots_are_preserved();
  test_unreachable_objects_are_collected();
  test_nested_objects_are_reached();
  test_cycles_are_supported();
  test_root_stack_grows();
  test_deep_graph_does_not_use_call_stack();
  test_strings_are_managed();

  if (failures != 0) {
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return EXIT_FAILURE;
  }

  puts("All garbage collector tests passed.");
  return EXIT_SUCCESS;
}
