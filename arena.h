#ifndef _ARENA_H 
#define _ARENA_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  uint32_t capacity;
  uint32_t length;
  uint8_t *data;
} Arena;

Arena *arena_new(uint32_t capacity);

Arena *arena_grow(Arena *arena);

void *arena_alloc(Arena *arena, uint32_t size);

void arena_reset(Arena *arena);

void arena_free(Arena *arena);

#endif //_ARENA_H 
