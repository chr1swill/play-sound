#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sqlite3.h>
#include "arena.h"

typedef struct {
  size_t capacity;
  size_t length;
  char **data;
} List;

List *list_new(Arena *a, size_t capacity) {
  List *list = arena_alloc(a, sizeof(List));
  if (list == NULL) return NULL;
  list->data = arena_alloc(a, sizeof(char *) *capacity);
  if (list->data == NULL) return NULL;
  list->capacity = capacity;
  list->length = 0;
  return list;
}

char *list_append(Arena *a, List *l, char *item) {
  if (l->length + 1 > l->capacity) return NULL;

  size_t str_len = strlen(item) + 1;
  char *item_dup = arena_alloc(a, str_len);
  if (item_dup == NULL) return NULL;

  memmove(item_dup, item, str_len);

  l->data[l->length] = item_dup;
  l->length++;
  return l->data[l->length-1];
}

List *tokenize(Arena *a, char *str, int len) {
  List *tokens = list_new(a, len);

  for (int i = 0; i < len; i++) {
    int char_code = str[i];

    if (isalpha((char)char_code) != 0) {
      int j = 0;
      while (isalpha(str[i+j]) != 0) j++;

      char *token = arena_alloc(a, sizeof(char) * j+1);
      if (token == NULL) {
        fprintf(stderr, "Error failed to allocate for word token\n");
        return NULL;
      }

      memmove(token, &str[i], j);
      token[j+1] = '\0';

      if (list_append(a, tokens, token) == NULL) {
        fprintf(stderr, "Error failed to append to list: %s\n", token);
        return NULL;
      }
      i += j-1;
      continue;
    }

    char *token = arena_alloc(a, sizeof(char) * 2);
    if (token == NULL) {
      fprintf(stderr, "Error failed to allocate for char token: %c\n", str[i]);
      return NULL;
    }

    memmove(token, &str[i], 1);
    token[1] = '\0';

    if (list_append(a, tokens, token) == NULL) {
      fprintf(stderr, "Error failed to append to list: %s\n", token);
      return NULL;
    }
    continue;
  }

  return tokens;
}

void ussage_message(char **argv){
  printf("Ussage:\n");
  printf("\t%s 'string your would like to be played'\n", argv[0]);
}

void to_upper(char *str, int len) {
  for (int i = 0; i < len; i++) {
    int char_code = (int)str[i];
    if (char_code >= 97 && char_code <= 122) str[i] -=  32;
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    ussage_message(argv);
    return 1;
  }

  char *str = argv[1];
  int str_length = strlen(str);
  to_upper(str, str_length);

  Arena *a = arena_new(2<<10);
  if (a == NULL) {
    fprintf(stderr, "Error creating arena\n");
    return 2;
  }

  List *tokens = tokenize(a, str, str_length);
  if (tokens == NULL) {
    fprintf(stderr, "Error could not tokenize string\n");
    return 3;
  }

  for (size_t i = 0; i < tokens->length; i++) {
    printf("tokens->data[%zu]   : (%s)\n", i, tokens->data[i]);
  }

  List *phonemes = list_new(a, tokens->capacity);
  if (tokens == NULL) {
    fprintf(stderr, "Error could not make List to hold phonemes\n");
    return 3;
  }

  int rc = -1;
  sqlite3 *db = NULL;

  rc = sqlite3_open_v2("g2p.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
  if (rc) {
    fprintf(stderr, "Can't open/create db: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    arena_free(a);
    exit(EXIT_FAILURE);
  }


  sqlite3_stmt *stmt;
  for (size_t i = 0; i < tokens->length; i++) {
    if (strncmp(tokens->data[i], " ", strlen(" ")) == 0) {
        if (list_append(a, phonemes, "SIL") == NULL) {
          fprintf(stderr, "Failed to append phoneme to phonemes list\n");
          sqlite3_close(db);
          arena_free(a);
          return 6;
        }
        continue;
    }

    if (strlen(tokens->data[i]) > 1) {
      if(sqlite3_prepare_v2(db,
          "select * from dict where word = ?;",
          -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "Failed to create prepared statment: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        arena_free(a);
        return 4;
      }

      if (sqlite3_bind_text(stmt, 1, tokens->data[i], -1, SQLITE_STATIC) != SQLITE_OK) {
        fprintf(stderr, "Failed to make statement with value: %s: %s\n", tokens->data[i], sqlite3_errmsg(db));
        sqlite3_close(db);
        arena_free(a);
        return 4;
      }
    } else {
      if(sqlite3_prepare_v2(db,
          "select * from dict where word like ?;",
          -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "Failed to create prepared statment: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        arena_free(a);
        return 4;
      }

      char *pattern = arena_alloc(a, strlen(tokens->data[i]) + 2);
      if (pattern == NULL) {
        fprintf(stderr, "Error could not allocate memory for pattern: %s\n", tokens->data[i]);
        sqlite3_close(db);
        arena_free(a);
        return 4;
      }

      pattern[strlen(tokens->data[i])] = '%';
      pattern[strlen(tokens->data[i])+1] = '\0';
      memmove(&pattern[0], tokens->data[i], strlen(tokens->data[i]));
      printf("pattern: %s\n", pattern);

      if (sqlite3_bind_text(stmt, 1, tokens->data[i], -1, SQLITE_STATIC) != SQLITE_OK) {
        fprintf(stderr, "Failed to make statement with value: %s: %s\n", pattern, sqlite3_errmsg(db));
        sqlite3_close(db);
        arena_free(a);
        return 4;
      }
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      char *phoneme = (char *)sqlite3_column_text(stmt, 1);
      if (list_append(a, phonemes, phoneme) == NULL) {
        fprintf(stderr, "Failed to append phoneme to phonemes list\n");
        sqlite3_close(db);
        arena_free(a);
        return 6;
      }
    }
  }

  printf("phonemes->length: %zu\n", phonemes->length); 
  for (size_t i = 0; i < phonemes->length; i++) {
    printf("phonemes->data[%zu]   : (%s)\n", i, phonemes->data[i]);
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  arena_free(a);
  return 0;
}
