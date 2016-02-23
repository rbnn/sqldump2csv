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

struct sql_context sql_context_init(void)
{
  struct sql_context new_ctx;
  new_ctx.compress = 0;
  new_ctx.dont_drop = 0;
  new_ctx.add_header = 0;
  new_ctx.add_types = 0;
  new_ctx.auto_close = 1;
  new_ctx.current_table = NULL;
  new_ctx.first_table = NULL;
  new_ctx.last_table = NULL;
  new_ctx.current_column = NULL;
  new_ctx.source_file = NULL;
  return new_ctx;
}

void sql_context_destroy(struct sql_context *p)
{
  sql_check_nullptr(p);

  sql_context_unlock_table(p);
  struct sql_table *it = p->first_table;
  while(NULL != it) {
    struct sql_table *it_next = it->next;
    sql_debug("Removing table `%s' from context...", it->name);
    sql_table_free(it);
    it = it_next;
  } /* for ... */
}

void sql_context_add_table(struct sql_context *p, struct sql_table *q)
{
  sql_check_nullptr(p);
  sql_check_nullptr(q);

  struct sql_table *it = p->first_table;
  for(; NULL != it; it = it->next) {
    if(0 == strcmp(q->name, it->name)) {
      /* Programmabbruch, da die Tabelle bereits existiert! */
      sql_die("Table `%s' already exists!", it->name);
    } /* if(0 == strcmp ... ) */
  } /* for ... */

  sql_debug("Adding table `%s' to context...", q->name);
  if(NULL == p->first_table) {
    /* Tabelle am Anfang einfügen */
    p->first_table = q;
  } /* if(NULL == ctx->first_table) */

  /* Tabelle am Ende einfügen */
  if(NULL != p->last_table) {
    p->last_table->next = q;
    q->prev = p->last_table;
  } /* if(NULL == ctx->last_table) */
  p->last_table = q;
}

void sql_context_lock_table(struct sql_context *p, const char *name)
{
  sql_check_nullptr(p);

  if(NULL != p->current_table) {
    /* Programmabbruch, da bereits eine Tabelle gewählt wurde! */
    sql_die("Context already has locked table `%s'!", p->current_table->name);
  } /* if(NULL != p->current_table) */

  struct sql_table *it = p->first_table;
  for(; NULL != it; it = it->next) {
    if(0 == strcmp(it->name, name)) {
      /* Tabelle wählen */
      p->current_table = it;
      p->current_column = it->first_column;
      sql_debug("Context has locked table `%s'.", it->name);
      return;
    } /* if(0 == strcmp ... ) */
  } /* for ... */

  sql_die("Could not lock table `%s'! No such table!", name);
}

void sql_context_unlock_table(struct sql_context *p)
{
  sql_check_nullptr(p);

  p->current_table = NULL;
  p->current_column = NULL;
}

void sql_context_write_current_row(struct sql_context *p)
{
  sql_check_nullptr(p);
  sql_check_nullptr(p->current_table);

  if(NULL == p->current_table->out) {
    /* Tabelle muss noch geöffnet werden! */
    sql_table_open(p->current_table, p);
  } /* if(NULL == p->current_table->out) */

  /* Zeile schreiben */
  sql_table_write_row(p->current_table);

  p->current_column = p->current_table->first_column;
  p->current_table->rows += 1;
}

struct sql_column *sql_context_get_current_column(struct sql_context *p)
{
  sql_check_nullptr(p);
  return p->current_column;
}

void sql_context_next_column(struct sql_context *p)
{
  sql_check_nullptr(p);
  if(NULL != p->current_column) {
    /* Nächste Spalte */
    p->current_column = p->current_column->next;
  } /* if(NULL != p->current_column) */
}
