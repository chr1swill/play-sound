#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
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
  if (l->length + 1 > l->capacity) {
    fprintf(stderr, "Error appending to list would exceed list capacity=%zu, size=%zu\n", l->capacity, l->length);
    return NULL;
  }

  size_t str_len = strlen(item) + 1;
  char *item_dup = arena_alloc(a, str_len);
  if (item_dup == NULL) {
    fprintf(stderr, "Failed to allocate memory of size=%zu in arena to hold item\n", str_len);
    return NULL;
  }

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

  Arena *a = arena_new(2<<14);
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
    char *token = tokens->data[i];
    int token_len = strlen(tokens->data[i]);
    assert(token_len > 0);

    // TODO use | instead of SIL for space char and do some sort of encoding like ||  = SIL SIL, ||| = SIL SIL SIL
    // aka the more | the longer the pause in sound
    if (token_len == 1 && isalpha(token[0]) == 0) {
      switch (token[0]) {
        case ' ':
          if (list_append(a, phonemes, "|") == NULL) {
            fprintf(stderr, "Failed to append phoneme to phonemes list\n");
            sqlite3_close(db);
            arena_free(a);
            return 6;
          }
          continue;
        case '!':
          if (list_append(a, phonemes, "||") == NULL) {
            fprintf(stderr, "Failed to append phoneme to phonemes list\n");
            sqlite3_close(db);
            arena_free(a);
            return 6;
          }
          continue;
        case '?':
          if (list_append(a, phonemes, "||") == NULL) {
            fprintf(stderr, "Failed to append phoneme to phonemes list\n");
            sqlite3_close(db);
            arena_free(a);
            return 6;
          }
          continue;
        case ':':
          if (list_append(a, phonemes, "||") == NULL) {
            fprintf(stderr, "Failed to append phoneme to phonemes list\n");
            sqlite3_close(db);
            arena_free(a);
            return 6;
          }
          continue;
        case ';':
          if (list_append(a, phonemes, "||") == NULL) {
            fprintf(stderr, "Failed to append phoneme to phonemes list\n");
            sqlite3_close(db);
            arena_free(a);
            return 6;
          }
          continue;
        case '.':
          if (list_append(a, phonemes, "||") == NULL) {
            fprintf(stderr, "Failed to append phoneme to phonemes list\n");
            sqlite3_close(db);
            arena_free(a);
            return 6;
          }
          continue;
        case ',':
          if (list_append(a, phonemes, "||") == NULL) {
            fprintf(stderr, "Failed to append phoneme to phonemes list\n");
            sqlite3_close(db);
            arena_free(a);
            return 6;
          }
          continue;
        default:;
      }
    }

    if (isalpha(token[0]) != 0) {
      if(sqlite3_prepare_v2(db,
          "select * from dict where word = ?;",
          -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "Failed to create prepared statment: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        arena_free(a);
        return 4;
      }

      if (sqlite3_bind_text(stmt, 1, token, -1, SQLITE_STATIC) != SQLITE_OK) {
        fprintf(stderr, "Failed to make statement with value: %s: %s\n", token, sqlite3_errmsg(db));
        sqlite3_close(db);
        arena_free(a);
        return 4;
      }
    } else {
      char *pre_format_sql = "select * from dict where word like '%s%%';"; 

      int sql_len = strlen(pre_format_sql) + token_len + 3;
      char *sql = arena_alloc(a, sql_len);
      if (sql == NULL) {
        fprintf(stderr, "Error could not allocate memory for pattern: %s\n", token);
        sqlite3_close(db);
        arena_free(a);
        return 4;
      }

      snprintf(sql, sql_len, pre_format_sql, token);

      if(sqlite3_prepare_v2(db,
            sql,-1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "Failed to create prepared statment: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        arena_free(a);
        return 4;
      }
    }

    printf("token=%s\n", token);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      char *phoneme = (char *)sqlite3_column_text(stmt, 1);
      printf("phoneme=%s\n", phoneme);
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
