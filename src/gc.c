#include "gc.h"

#include <stdlib.h>
#include <string.h>

#define GC_INITIAL_STACK_CAPACITY 16
#define GC_INITIAL_THRESHOLD 8

struct GcObject {
  GcObjectType type;
  bool marked;
  struct GcObject* next;

  union {
    int value;
    char* string;
    struct {
      struct GcObject* head;
      struct GcObject* tail;
    } pair;
  } as;
};

struct GcVm {
  GcObject** stack;
  size_t stack_size;
  size_t stack_capacity;
  GcObject* first_object;
  size_t object_count;
  size_t threshold;
};

static bool ensure_stack_capacity(GcVm* vm, size_t required) {
  GcObject** stack;
  size_t capacity;

  if (required <= vm->stack_capacity) {
    return true;
  }

  capacity = vm->stack_capacity == 0
      ? GC_INITIAL_STACK_CAPACITY
      : vm->stack_capacity * 2;
  while (capacity < required) {
    capacity *= 2;
  }

  stack = (GcObject**)realloc(vm->stack, capacity * sizeof(GcObject*));
  if (stack == NULL) {
    return false;
  }

  vm->stack = stack;
  vm->stack_capacity = capacity;
  return true;
}

static void mark_roots(GcVm* vm) {
  size_t index;
  bool changed;

  for (index = 0; index < vm->stack_size; index++) {
    if (vm->stack[index] != NULL) {
      vm->stack[index]->marked = true;
    }
  }

  /*
   * Propagate marks iteratively. This intentionally trades some speed for a
   * collector that does not consume the C call stack on deeply nested graphs.
   */
  do {
    GcObject* object;
    changed = false;

    for (object = vm->first_object; object != NULL; object = object->next) {
      if (object->marked && object->type == GC_OBJ_PAIR) {
        GcObject* head = object->as.pair.head;
        GcObject* tail = object->as.pair.tail;

        if (head != NULL && !head->marked) {
          head->marked = true;
          changed = true;
        }
        if (tail != NULL && !tail->marked) {
          tail->marked = true;
          changed = true;
        }
      }
    }
  } while (changed);
}

static void clear_marks(GcVm* vm) {
  GcObject* object;
  for (object = vm->first_object; object != NULL; object = object->next) {
    object->marked = false;
  }
}

static size_t sweep(GcVm* vm) {
  GcObject** object = &vm->first_object;
  size_t collected = 0;

  while (*object != NULL) {
    if (!(*object)->marked) {
      GcObject* unreachable = *object;
      *object = unreachable->next;
      if (unreachable->type == GC_OBJ_STRING) {
        free(unreachable->as.string);
      }
      free(unreachable);
      vm->object_count--;
      collected++;
    } else {
      (*object)->marked = false;
      object = &(*object)->next;
    }
  }

  return collected;
}

static GcObject* new_object(GcVm* vm, GcObjectType type) {
  GcObject* object;

  if (vm == NULL) {
    return NULL;
  }

  if (vm->object_count >= vm->threshold) {
    gc_collect(vm);
  }

  object = (GcObject*)calloc(1, sizeof(GcObject));
  if (object == NULL) {
    return NULL;
  }

  object->type = type;
  object->next = vm->first_object;
  vm->first_object = object;
  vm->object_count++;
  return object;
}

GcVm* gc_vm_new(void) {
  GcVm* vm = (GcVm*)calloc(1, sizeof(GcVm));
  if (vm != NULL) {
    vm->threshold = GC_INITIAL_THRESHOLD;
  }
  return vm;
}

void gc_vm_free(GcVm* vm) {
  if (vm == NULL) {
    return;
  }

  vm->stack_size = 0;
  gc_collect(vm);
  free(vm->stack);
  free(vm);
}

bool gc_push(GcVm* vm, GcObject* object) {
  if (vm == NULL || !ensure_stack_capacity(vm, vm->stack_size + 1)) {
    return false;
  }

  vm->stack[vm->stack_size++] = object;
  return true;
}

GcObject* gc_pop(GcVm* vm) {
  if (vm == NULL || vm->stack_size == 0) {
    return NULL;
  }

  return vm->stack[--vm->stack_size];
}

bool gc_push_int(GcVm* vm, int value) {
  GcObject* object;

  if (vm == NULL || !ensure_stack_capacity(vm, vm->stack_size + 1)) {
    return false;
  }

  object = new_object(vm, GC_OBJ_INT);
  if (object == NULL) {
    return false;
  }

  object->as.value = value;
  if (!gc_push(vm, object)) {
    return false;
  }
  return true;
}

bool gc_push_string(GcVm* vm, const char* value) {
  GcObject* object;
  char* copy;
  size_t length;

  if (vm == NULL || value == NULL ||
      !ensure_stack_capacity(vm, vm->stack_size + 1)) {
    return false;
  }

  length = strlen(value);
  copy = (char*)malloc(length + 1);
  if (copy == NULL) {
    return false;
  }
  memcpy(copy, value, length + 1);

  object = new_object(vm, GC_OBJ_STRING);
  if (object == NULL) {
    free(copy);
    return false;
  }

  object->as.string = copy;
  if (!gc_push(vm, object)) {
    return false;
  }
  return true;
}

GcObject* gc_push_pair(GcVm* vm) {
  GcObject* object;
  GcObject* tail;
  GcObject* head;

  if (vm == NULL || vm->stack_size < 2) {
    return NULL;
  }

  if (!ensure_stack_capacity(vm, vm->stack_size + 1)) {
    return NULL;
  }

  object = new_object(vm, GC_OBJ_PAIR);
  if (object == NULL) {
    return NULL;
  }

  tail = gc_pop(vm);
  head = gc_pop(vm);
  object->as.pair.head = head;
  object->as.pair.tail = tail;
  gc_push(vm, object);
  return object;
}

size_t gc_collect(GcVm* vm) {
  size_t collected;

  if (vm == NULL) {
    return 0;
  }

  clear_marks(vm);
  mark_roots(vm);
  collected = sweep(vm);
  vm->threshold = vm->object_count == 0
      ? GC_INITIAL_THRESHOLD
      : vm->object_count * 2;
  return collected;
}

size_t gc_object_count(const GcVm* vm) {
  return vm == NULL ? 0 : vm->object_count;
}

size_t gc_threshold(const GcVm* vm) {
  return vm == NULL ? 0 : vm->threshold;
}

size_t gc_stack_size(const GcVm* vm) {
  return vm == NULL ? 0 : vm->stack_size;
}

size_t gc_stack_capacity(const GcVm* vm) {
  return vm == NULL ? 0 : vm->stack_capacity;
}

GcObjectType gc_object_type(const GcObject* object) {
  return object == NULL ? GC_OBJ_INT : object->type;
}

int gc_int_value(const GcObject* object) {
  return object == NULL || object->type != GC_OBJ_INT ? 0 : object->as.value;
}

const char* gc_string_value(const GcObject* object) {
  return object == NULL || object->type != GC_OBJ_STRING
      ? NULL
      : object->as.string;
}

GcObject* gc_pair_head(const GcObject* object) {
  return object == NULL || object->type != GC_OBJ_PAIR
      ? NULL
      : object->as.pair.head;
}

GcObject* gc_pair_tail(const GcObject* object) {
  return object == NULL || object->type != GC_OBJ_PAIR
      ? NULL
      : object->as.pair.tail;
}

void gc_pair_set_head(GcObject* pair, GcObject* head) {
  if (pair != NULL && pair->type == GC_OBJ_PAIR) {
    pair->as.pair.head = head;
  }
}

void gc_pair_set_tail(GcObject* pair, GcObject* tail) {
  if (pair != NULL && pair->type == GC_OBJ_PAIR) {
    pair->as.pair.tail = tail;
  }
}

void gc_object_print(const GcObject* object, FILE* stream) {
  if (object == NULL || stream == NULL) {
    return;
  }

  if (object->type == GC_OBJ_INT) {
    fprintf(stream, "%d", object->as.value);
    return;
  }

  if (object->type == GC_OBJ_STRING) {
    fprintf(stream, "\"%s\"", object->as.string);
    return;
  }

  fputc('(', stream);
  gc_object_print(object->as.pair.head, stream);
  fputs(", ", stream);
  gc_object_print(object->as.pair.tail, stream);
  fputc(')', stream);
}
