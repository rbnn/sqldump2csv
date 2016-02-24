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

#include "sql.h"
#include <limits.h>
#include <unistd.h>
#ifdef SQL_ZLIB
#include <zlib.h>
#endif /* SQL_ZLIB */

struct sql_table *sql_table_new(void)
{
  struct sql_table *tab = (struct sql_table*)sql_xmalloc(sizeof(struct sql_table));
  tab->out = NULL;
  tab->name = NULL;
  tab->filename = NULL;
  tab->first_column = NULL;
  tab->last_column = NULL;
  tab->prev = NULL;
  tab->next = NULL;
  tab->rows = 0;
  tab->drop_data = 0;
  return tab;
}

void sql_table_free(struct sql_table *p)
{
  if(NULL != p) {
    sql_table_close(p);
    sql_table_set_name(p, NULL);
    sql_table_set_file(p, NULL);
    sql_table_del_sibbling(p);

    /* Spalten löschen */
    struct sql_column *it = p->first_column;
    while(NULL != it) {
      struct sql_column *it_next = it->next;
      sql_column_free(it);
      it = it_next;
    } /* while ... */

    sql_xfree(p);
  } /* if(NULL != p) */
}

void sql_table_set_name(struct sql_table *p, const char *name)
{
  sql_check_nullptr(p);

  if(NULL != p->name) {
    /* Alten Namen löschen */
    sql_xfree(p->name);
    p->name = NULL;
  } /* if(NULL != p->name) */

  p->name = sql_xstrdup(name);
}

void sql_table_set_file(struct sql_table *p, const char *name)
{
  sql_check_nullptr(p);

  if(NULL != p->filename) {
    /* Alten Namen löschen */
    sql_xfree(p->filename);
    p->filename = NULL;
  } /* if(NULL != p->name) */

  p->filename = sql_xstrdup(name);
}

void sql_table_open(struct sql_table *p, const struct sql_context *q)
{
  sql_check_nullptr(p);
  sql_check_nullptr(q);

  if(NULL != p->out) {
    /* Die Tabelle wurde bereits geöffnet */
    sql_debug("Table `%s' was already opened as file `%s'!", p->name, p->filename);
    return;
  } /* if(NULL != p->out) */

  char tmp_filename[2 * PATH_MAX] = {0};
  if(NULL == q->out_dir) {
    snprintf(tmp_filename, sizeof(tmp_filename), "%s.%s.csv%s", q->source_file, p->name, q->compress ? ".gz" : "");
  } else {
    snprintf(tmp_filename, sizeof(tmp_filename), "%s/%s.%s.csv%s", q->out_dir, q->source_file, p->name, q->compress ? ".gz" : "");
  } /* if(NULL == q->out_dir) */
  sql_table_set_file(p, tmp_filename);
  sql_debug("Opening table `%s' as file `%s'...", p->name, p->filename);

  /* Der Header soll erzeugt werden, wenn:
   * 1. Die Tabelle noch nicht existiert
   * 2. oder verworfen werden soll.
   */
  const int allow_header = !(0 == access(p->filename, W_OK)) || !q->dont_drop;
  const char *mode = q->dont_drop ? "a" : "w";
  p->float_fmt = q->float_fmt;

  if(q->compress) {
    #ifdef SQL_ZLIB
    gzFile zf = Z_NULL;
    if(NULL == (zf = gzopen(p->filename, mode))) {
      /* Die Datei kann nicht geöffnet werden. */
      sql_die("Could not open compressed file `%s'!", p->filename);
    } /* if(NULL == ... gzopen(...)) */
    const cookie_io_functions_t cfunc = {
      (cookie_read_function_t*)gzread,
      (cookie_write_function_t*)gzwrite,
      (cookie_seek_function_t*)gzseek,
      (cookie_close_function_t*)gzclose};
    if(NULL == (p->out = fopencookie(zf, mode, cfunc))) {
      /* Fehler, da die Datei nicht geöffnet wurde! */
      gzclose(zf);
      sql_die("Could not open compressed file `%s'!", p->filename);
    } /* if(NULL == ...fopencookie(...)) */
    #else /* SQL_ZLIB */
    sql_die("Program was compiled without compression!");
    #endif /* SQL_ZLIB */
  } else {
    if(NULL == (p->out = fopen(p->filename, mode))) {
      /* Programmabbruch, da die Datei nicht geöffnet werden konnte! */
      sql_die("Could not open file `%s'! (Error: %m)", p->filename);
    } /* if(NULL == ... fopen ... ) */
  } /* if(p->compress) */

  sql_check_nullptr(p->out);
  p->rows = 0;

  if(q->add_header && allow_header) {
    /* Header hinzufügen */
    sql_table_write_header(p);
  } /* if(q->add_header) */

  if(q->add_types && allow_header) {
    /* Typen hinzufügen */
    sql_table_write_types(p);
  } /* if(q->add_types) */
}

void sql_table_write_header(struct sql_table *p)
{
  sql_check_nullptr(p);
  sql_check_nullptr(p->out);

  int is_first_column = 1;
  struct sql_column *it = p->first_column;

  for(; NULL != it; it = it->next) {
    if(!is_first_column) {
      fprintf(p->out, ",");
    } /* if(!is_first_column) */
    is_first_column = 0;

    fprintf(p->out, "%s", it->name);
  } /* for ... */

  fprintf(p->out, "\n");
}

void sql_table_write_types(struct sql_table *p)
{
  sql_check_nullptr(p);
  sql_check_nullptr(p->out);

  int is_first_column = 1;
  struct sql_column *it = p->first_column;

  for(; NULL != it; it = it->next) {
    if(is_first_column) {
      fprintf(p->out, "# ");
    } else {
      fprintf(p->out, ",");
    } /* if(is_first_column) */
    is_first_column = 0;

    fprintf(p->out, "%s:", it->name);
    switch(it->type) {
      case sql_column_type_none:
        fprintf(p->out, "none");
        break;
      case sql_column_type_int:
        fprintf(p->out, "int");
        break;
      case sql_column_type_float:
        fprintf(p->out, "float");
        break;
      case sql_column_type_str:
        fprintf(p->out, "string");
        break;
      default:
        /* Programmabbruch, da der Typ unbekannt ist! */
        sql_die_invalid_type(it);
    } /* switch(p->type) */
  } /* for ... */

  fprintf(p->out, "\n");
}
void sql_table_write_row(struct sql_table *p)
{
  sql_check_nullptr(p);
  sql_check_nullptr(p->out);

  int is_first_column = 1;
  struct sql_column *it = p->first_column;

  for(; NULL != it; it = it->next) {
    if(!is_first_column) {
      fprintf(p->out, ",");
    } /* if(!is_first_column) */
    is_first_column = 0;

    switch(it->type) {
      case sql_column_type_int:
        fprintf(p->out, "%lli", it->int_value);
        break;
      case sql_column_type_float:
        fprintf(p->out, p->float_fmt, it->flt_value);
        break;
      default:
        /* Programmabbruch, da der Typ unbekannt ist! */
        sql_die_invalid_type(it);
    } /* switch(p->type) */
  } /* for ... */

  fprintf(p->out, "\n");
}

void sql_table_close(struct sql_table *p)
{
  sql_check_nullptr(p);

  if(NULL == p->out) {
    /* Datei wurde bereits geschlossen */
    sql_debug("Table `%s' has already been closed!", p->name);
  } else {
    fclose(p->out);
    p->out = NULL;
  } /* if(NULL == p->out) */
}

void sql_table_del_sibbling(struct sql_table *p)
{
  sql_check_nullptr(p);

  if(NULL != p->prev) {
    p->prev->next = p->next;
  } /* if(NULL != p->prev) */

  if(NULL != p->next) {
    p->next->prev = p->prev;
  } /* if(NULL != p->next) */
}
