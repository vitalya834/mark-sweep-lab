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

static void test_statistics_and_threshold(void) {
  GcVm* vm = gc_vm_new();
  GcStats stats;

  gc_set_threshold(vm, 64);
  gc_push_int(vm, 1);
  gc_push_string(vm, "two");
  gc_pop(vm);
  gc_collect(vm);

  stats = gc_get_stats(vm);
  check(stats.allocations == 2, "statistics count allocations");
  check(stats.collections == 1, "statistics count collections");
  check(stats.collected == 1, "statistics count collected objects");
  check(stats.live_objects == 1, "statistics report live objects");
  check(stats.roots == 1, "statistics report roots");
  check(stats.threshold == 64, "configured minimum threshold is retained");
  gc_vm_free(vm);
}

static void test_root_scopes_release_temporaries(void) {
  GcVm* vm = gc_vm_new();
  GcRootScope scope;

  gc_push_int(vm, 1);
  scope = gc_scope_begin(vm);
  gc_push_string(vm, "temporary");
  gc_push_int(vm, 3);

  check(gc_peek(vm, 0) != NULL, "peek returns the newest root");
  check(gc_peek(vm, 2) != NULL, "peek reaches an older root");
  check(gc_peek(vm, 3) == NULL, "peek rejects an invalid distance");

  gc_scope_end(vm, scope);
  check(gc_stack_size(vm) == 1, "ending scope restores root count");
  check(gc_collect(vm) == 2, "scope temporaries become collectible");
  check(gc_object_count(vm) == 1, "outer root stays alive");
  gc_vm_free(vm);
}

static void test_weak_references_do_not_keep_objects_alive(void) {
  GcVm* vm = gc_vm_new();
  GcObject* object;
  GcWeakRef* weak_ref;

  gc_push_string(vm, "cached value");
  object = gc_peek(vm, 0);
  weak_ref = gc_weak_ref_new(vm, object);

  check(weak_ref != NULL, "weak reference allocation succeeds");
  check(gc_weak_ref_get(weak_ref) == object, "weak reference exposes target");

  gc_pop(vm);
  check(gc_collect(vm) == 1, "weak target remains collectible");
  check(gc_weak_ref_get(weak_ref) == NULL, "collected weak target is cleared");

  gc_weak_ref_free(vm, weak_ref);
  gc_vm_free(vm);
}

static void test_heap_graph_export(void) {
  GcVm* vm = gc_vm_new();
  FILE* stream = tmpfile();
  char output[1024] = {0};
  size_t bytes_read;

  gc_push_string(vm, "left");
  gc_push_int(vm, 42);
  gc_push_pair(vm);
  gc_heap_dump_dot(vm, stream);

  rewind(stream);
  bytes_read = fread(output, 1, sizeof(output) - 1, stream);
  output[bytes_read] = '\0';

  check(strstr(output, "digraph heap") != NULL, "DOT output has graph header");
  check(strstr(output, "string: left") != NULL, "DOT output labels strings");
  check(strstr(output, "int: 42") != NULL, "DOT output labels integers");
  check(strstr(output, "label=\"head\"") != NULL, "DOT output includes edges");

  fclose(stream);
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
  test_statistics_and_threshold();
  test_root_scopes_release_temporaries();
  test_weak_references_do_not_keep_objects_alive();
  test_heap_graph_export();

  if (failures != 0) {
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return EXIT_FAILURE;
  }

  puts("All garbage collector tests passed.");
  return EXIT_SUCCESS;
}
