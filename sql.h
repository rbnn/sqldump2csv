/* The MIT License (MIT)
 * 
 * Copyright (c) 2016 rbnn
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _SQL_UTILS_H_
#define _SQL_UTILS_H_
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define sql_assert   assert
#define sql_check_nullptr(x)  sql_assert(NULL != x)
void *sql_xmalloc(size_t s);
void sql_xfree(void *p);
char *sql_xstrdup(const char *s);

extern int sql_be_quiet;

#ifdef SQL_DEBUG
#define sql_debug(...)  {\
  if(!sql_be_quiet) {\
    fprintf(stderr, "%s:%i: Debug: ", __FILE__, __LINE__);\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, "\n");\
    fflush(stderr);\
  }}
#else /* SQL_DEBUG */
#define sql_debug(...)
#endif /* SQL_DEBUG */

#define sql_warning(...)  {\
  if(!sql_be_quiet) { \
    fprintf(stderr, "%s:%i: Warning: ", __FILE__, __LINE__);\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, "\n");\
    fflush(stderr);\
  }}

#define sql_error(...)  {\
  fprintf(stderr, "%s:%i: Error: ", __FILE__, __LINE__);\
  fprintf(stderr, __VA_ARGS__);\
  fprintf(stderr, "\n");\
  fflush(stderr);}

#define sql_die(...)  {\
  fprintf(stderr, "%s:%i: Fatal: ", __FILE__, __LINE__);\
  fprintf(stderr, __VA_ARGS__);\
  fprintf(stderr, "\n");\
  fflush(stderr);\
  abort();}

#define sql_die_invalid_type(p) sql_die("Column %p(%s) has invalid type!", p, (NULL != p) ? p->name : "<nil>")

struct sql_context;

enum sql_column_type {
  sql_column_type_none,
  sql_column_type_int,
  sql_column_type_float,
  sql_column_type_str
}; /* enum sql_column_type */

struct sql_column {
  char  *name;
  enum sql_column_type type;
  union {
    long long   int_value;
    long double flt_value;
    char       *str_value;
  }; /* values */
  struct sql_column   *prev;
  struct sql_column   *next;
}; /* struct sql_column */

struct sql_column *sql_column_new(void);
void sql_column_free(struct sql_column *p);
void sql_column_set_name(struct sql_column *p, const char *name);
void sql_column_set_none(struct sql_column *p);
void sql_column_set_int(struct sql_column *p, const long long x);
void sql_column_set_float(struct sql_column *p, const long double x);
void sql_column_set_string(struct sql_column *p, const char *x);
void sql_column_add_sibbling(struct sql_column *p, struct sql_column *q, int pos);
void sql_column_del_sibbling(struct sql_column *p);
struct sql_column *sql_column_get_first_sibbling(struct sql_column *p);
struct sql_column *sql_column_get_last_sibbling(struct sql_column *p);

struct sql_table {
  FILE              *out;
  char              *name;
  char              *filename;
  struct sql_column *first_column;
  struct sql_column *last_column;
  struct sql_table  *prev;
  struct sql_table  *next;
  size_t             rows;
  struct  {
    int drop_data:1;
  }; /* flags */
};

struct sql_table *sql_table_new(void);
void sql_table_free(struct sql_table *p);
void sql_table_set_name(struct sql_table *p, const char *name);
void sql_table_set_file(struct sql_table *p, const char *name);
void sql_table_open(struct sql_table *p, const struct sql_context *q);
void sql_table_write_header(struct sql_table *p);
void sql_table_write_types(struct sql_table *p);
void sql_table_write_row(struct sql_table *p);
void sql_table_close(struct sql_table *p);
void sql_table_add_column(struct sql_table *p, struct sql_column *q);
void sql_table_add_sibbling(struct sql_table *p, struct sql_table *q, int pos);
void sql_table_del_sibbling(struct sql_table *p);
struct sql_table *sql_table_get_first_sibbling(struct sql_table *p);
struct sql_table *sql_table_get_last_sibbling(struct sql_table *p);

struct sql_context {
  struct {
    int compress:  1;
    int dont_drop: 1;
    int add_header:1;
    int add_types: 1;
    int auto_close:1;
  }; /* options */
  /* -- Tables -- */
  struct sql_table  *current_table;
  struct sql_table  *first_table;
  struct sql_table  *last_table;
  /* -- Columns -- */
  struct sql_column *current_column;
  /* -- Sonstiges -- */
  char *source_file;
}; /* struct sql_context */

struct sql_context sql_context_init(void);
void sql_context_destroy(struct sql_context *p);
void sql_context_add_table(struct sql_context *p, struct sql_table *q);
void sql_context_lock_table(struct sql_context *p, const char *name);
void sql_context_unlock_table(struct sql_context *p);
// struct sql_column *sql_context_get_current_row(struct sql_context *p);
void sql_context_write_current_row(struct sql_context *p);

struct sql_column *sql_context_get_current_column(struct sql_context *p);
void sql_context_next_column(struct sql_context *p);

// void sql_context_close_table(struct sql_context *ctx);
#endif /* _SQL_UTILS_H_ */
