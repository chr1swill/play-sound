#include "arena.h"

Arena *arena_new(uint32_t capacity) {
  void *data = malloc(capacity);
  if (data == NULL) {
    fprintf(stderr, "Error could not allocate requested memory for arena: %d\n", capacity); 
    return NULL;
  }
  memset(data, 0, capacity);

  Arena *a = malloc(sizeof(Arena));
  if (a == NULL) {
    fprintf(stderr, "Error could not allocate structure arena\n"); 
    free(data);
    return NULL;
  }
  memset(a, 0, sizeof(Arena));

  a->capacity = capacity;
  a->length = 0;
  a->data = data;
  return a;
}

Arena *arena_grow(Arena *arena);

void *arena_alloc(Arena *arena, uint32_t size) {
  if (arena->length + size > arena->capacity) {
    // Change to later auto grow the arena by double or something
    fprintf(stderr,
        "Error you have requested size %d will not fit inside of arena of capacity %d and current size %d\n", 
        size, arena->capacity, arena->length); 
    return NULL;
  }
  void *memory = &arena->data[arena->length];
  arena->length += size;
  return memory;
}

void arena_reset(Arena *arena) {
  // may not even need to do this
  // will be over wrote anyways
  memset(arena->data, 0, arena->length); 
  arena->length = 0;
  return;
}

void arena_free(Arena *arena) {
  free(arena->data);
  free(arena);
}
