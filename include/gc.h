#ifndef MARK_SWEEP_GC_H
#define MARK_SWEEP_GC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct GcObject GcObject;
typedef struct GcVm GcVm;

typedef enum {
  GC_OBJ_INT,
  GC_OBJ_PAIR
} GcObjectType;

GcVm* gc_vm_new(void);
void gc_vm_free(GcVm* vm);

bool gc_push(GcVm* vm, GcObject* object);
GcObject* gc_pop(GcVm* vm);
bool gc_push_int(GcVm* vm, int value);
GcObject* gc_push_pair(GcVm* vm);

size_t gc_collect(GcVm* vm);
size_t gc_object_count(const GcVm* vm);
size_t gc_threshold(const GcVm* vm);
size_t gc_stack_size(const GcVm* vm);

GcObjectType gc_object_type(const GcObject* object);
int gc_int_value(const GcObject* object);
GcObject* gc_pair_head(const GcObject* object);
GcObject* gc_pair_tail(const GcObject* object);
void gc_pair_set_head(GcObject* pair, GcObject* head);
void gc_pair_set_tail(GcObject* pair, GcObject* tail);
void gc_object_print(const GcObject* object, FILE* stream);

#endif
