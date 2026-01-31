#ifndef MARK_SWEEP_GC_H
#define MARK_SWEEP_GC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct GcObject GcObject;
typedef struct GcVm GcVm;

typedef enum {
  GC_OBJ_INT,
  GC_OBJ_PAIR,
  GC_OBJ_STRING
} GcObjectType;

typedef struct {
  size_t allocations;
  size_t collections;
  size_t collected;
  size_t live_objects;
  size_t threshold;
  size_t roots;
} GcStats;

typedef struct {
  size_t root_count;
} GcRootScope;

GcVm* gc_vm_new(void);
void gc_vm_free(GcVm* vm);

bool gc_push(GcVm* vm, GcObject* object);
GcObject* gc_pop(GcVm* vm);
GcObject* gc_peek(const GcVm* vm, size_t distance);
GcRootScope gc_scope_begin(const GcVm* vm);
void gc_scope_end(GcVm* vm, GcRootScope scope);
bool gc_push_int(GcVm* vm, int value);
bool gc_push_string(GcVm* vm, const char* value);
GcObject* gc_push_pair(GcVm* vm);

size_t gc_collect(GcVm* vm);
size_t gc_object_count(const GcVm* vm);
size_t gc_threshold(const GcVm* vm);
size_t gc_stack_size(const GcVm* vm);
size_t gc_stack_capacity(const GcVm* vm);
void gc_set_threshold(GcVm* vm, size_t threshold);
GcStats gc_get_stats(const GcVm* vm);

GcObjectType gc_object_type(const GcObject* object);
int gc_int_value(const GcObject* object);
const char* gc_string_value(const GcObject* object);
GcObject* gc_pair_head(const GcObject* object);
GcObject* gc_pair_tail(const GcObject* object);
void gc_pair_set_head(GcObject* pair, GcObject* head);
void gc_pair_set_tail(GcObject* pair, GcObject* tail);
void gc_object_print(const GcObject* object, FILE* stream);

#endif
