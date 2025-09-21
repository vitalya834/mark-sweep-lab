#include "gc.h"

#include <stdio.h>

int main(void) {
  GcVm* vm = gc_vm_new();
  GcObject* root;
  GcStats stats;

  if (vm == NULL ||
      !gc_push_int(vm, 1) ||
      !gc_push_int(vm, 2)) {
    fputs("Could not initialize demo.\n", stderr);
    gc_vm_free(vm);
    return 1;
  }

  root = gc_push_pair(vm);
  if (root == NULL) {
    fputs("Could not allocate pair.\n", stderr);
    gc_vm_free(vm);
    return 1;
  }

  fputs("Root: ", stdout);
  gc_object_print(root, stdout);
  fputc('\n', stdout);
  printf("Objects before collection: %zu\n", gc_object_count(vm));
  printf("Collected: %zu\n", gc_collect(vm));
  printf("Objects after collection: %zu\n", gc_object_count(vm));
  stats = gc_get_stats(vm);
  printf("Lifetime allocations: %zu, collections: %zu\n",
         stats.allocations, stats.collections);

  gc_vm_free(vm);
  return 0;
}
