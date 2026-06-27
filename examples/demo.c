#include "gc.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  size_t iterations;
  size_t threshold;
  const char* dot_path;
} Options;

static void print_usage(const char* program) {
  printf(
      "Usage: %s [options]\n"
      "\n"
      "Options:\n"
      "  --iterations N  Allocate N temporary object graphs (default: 10000)\n"
      "  --threshold N   Trigger collection after N live objects (default: 64)\n"
      "  --dot FILE      Write the surviving heap graph as Graphviz DOT\n"
      "  --help          Show this help\n",
      program);
}

static bool parse_size(const char* text, size_t* value) {
  char* end;
  unsigned long parsed;

  errno = 0;
  parsed = strtoul(text, &end, 10);
  if (errno != 0 || *text == '\0' || *end != '\0') {
    return false;
  }

  *value = (size_t)parsed;
  return true;
}

static bool parse_options(int argc, char** argv, Options* options) {
  int index;

  options->iterations = 10000;
  options->threshold = 64;
  options->dot_path = NULL;

  for (index = 1; index < argc; index++) {
    if (strcmp(argv[index], "--help") == 0) {
      print_usage(argv[0]);
      exit(EXIT_SUCCESS);
    }

    if (strcmp(argv[index], "--iterations") == 0 ||
        strcmp(argv[index], "--threshold") == 0 ||
        strcmp(argv[index], "--dot") == 0) {
      const char* option = argv[index];
      if (++index >= argc) {
        fprintf(stderr, "Missing value for %s.\n", option);
        return false;
      }

      if (strcmp(option, "--iterations") == 0) {
        if (!parse_size(argv[index], &options->iterations)) {
          fputs("Invalid iteration count.\n", stderr);
          return false;
        }
      } else if (strcmp(option, "--threshold") == 0) {
        if (!parse_size(argv[index], &options->threshold) ||
            options->threshold == 0) {
          fputs("Threshold must be greater than zero.\n", stderr);
          return false;
        }
      } else {
        options->dot_path = argv[index];
      }
      continue;
    }

    fprintf(stderr, "Unknown option: %s\n", argv[index]);
    return false;
  }

  return true;
}

static bool build_sample_root(GcVm* vm) {
  return gc_push_int(vm, 1) &&
      gc_push_string(vm, "two") &&
      gc_push_pair(vm) != NULL;
}

static bool run_stress_test(GcVm* vm, size_t iterations) {
  size_t index;

  for (index = 0; index < iterations; index++) {
    GcRootScope scope = gc_scope_begin(vm);
    if (!gc_push_int(vm, (int)index) ||
        !gc_push_string(vm, "temporary") ||
        gc_push_pair(vm) == NULL) {
      return false;
    }
    gc_scope_end(vm, scope);
  }

  gc_collect(vm);
  return true;
}

static bool write_dot_file(const GcVm* vm, const char* path) {
  FILE* stream = fopen(path, "w");
  if (stream == NULL) {
    perror(path);
    return false;
  }

  gc_heap_dump_dot(vm, stream);
  if (fclose(stream) != 0) {
    perror(path);
    return false;
  }
  return true;
}

int main(int argc, char** argv) {
  Options options;
  GcVm* vm;
  GcStats stats;

  if (!parse_options(argc, argv, &options)) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  vm = gc_vm_new();
  if (vm == NULL) {
    fputs("Could not allocate the VM.\n", stderr);
    return EXIT_FAILURE;
  }

  gc_set_threshold(vm, options.threshold);
  if (!build_sample_root(vm) ||
      !run_stress_test(vm, options.iterations)) {
    fputs("Object allocation failed.\n", stderr);
    gc_vm_free(vm);
    return EXIT_FAILURE;
  }

  if (options.dot_path != NULL && !write_dot_file(vm, options.dot_path)) {
    gc_vm_free(vm);
    return EXIT_FAILURE;
  }

  fputs("Root: ", stdout);
  gc_object_print(gc_peek(vm, 0), stdout);
  fputc('\n', stdout);

  stats = gc_get_stats(vm);
  printf("Allocations: %zu\n", stats.allocations);
  printf("Collections: %zu\n", stats.collections);
  printf("Collected: %zu\n", stats.collected);
  printf("Live objects: %zu\n", stats.live_objects);
  printf("Next threshold: %zu\n", stats.threshold);

  gc_vm_free(vm);
  return EXIT_SUCCESS;
}
