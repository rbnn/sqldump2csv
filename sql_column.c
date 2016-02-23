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

struct sql_column *sql_column_new(void)
{
  struct sql_column *col = (struct sql_column*)sql_xmalloc(sizeof(struct sql_column));
  col->name = NULL;
  col->type = sql_column_type_none;
  col->prev = NULL;
  col->next = NULL;
  return col;
}

void sql_column_free(struct sql_column *p)
{
  if(NULL != p) {
    sql_column_set_name(p, NULL);
    sql_column_set_none(p);
    sql_column_del_sibbling(p);
    sql_xfree(p);
  } /* if(NULL != p) */
}

void sql_column_set_name(struct sql_column *p, const char *name)
{
  sql_check_nullptr(p);
  if(NULL != p->name) {
    /* Alten Namen löschen */
    sql_xfree(p->name);
    p->name = NULL;
  } /* if(NULL != p->name) */
  
  p->name = sql_xstrdup(name);
}

void sql_column_set_none(struct sql_column *p)
{
  sql_check_nullptr(p);

  switch(p->type) {
    case sql_column_type_none:
    case sql_column_type_int:
    case sql_column_type_float:
      /* Nix weiter */
      break;
    case sql_column_type_str:
      sql_xfree(p->str_value);
      break;
    default:
      /* Ungültiger typ! */
      sql_die_invalid_type(p);
  }; /* switch(p->type) */

  p->type = sql_column_type_none;
}

void sql_column_set_int(struct sql_column *p, const long long x)
{
  sql_check_nullptr(p);
  
  switch(p->type) {
    case sql_column_type_none:
    case sql_column_type_int:
    case sql_column_type_float:
      /* Nix weiter */
      break;
    case sql_column_type_str:
      /* Den Speicher frei geben */
      sql_column_set_none(p);
      break;
    default:
      /* Ungültiger typ! */
      sql_die_invalid_type(p);
  }; /* switch(p->type) */

  p->type = sql_column_type_int;
  p->int_value = x;
}

void sql_column_set_float(struct sql_column *p, const long double x)
{
  sql_check_nullptr(p);
  
  switch(p->type) {
    case sql_column_type_none:
    case sql_column_type_int:
    case sql_column_type_float:
      /* Nix weiter */
      break;
    case sql_column_type_str:
      /* Den Speicher frei geben */
      sql_column_set_none(p);
      break;
    default:
      /* Ungültiger typ! */
      sql_die_invalid_type(p);
  }; /* switch(p->type) */

  p->type = sql_column_type_float;
  p->flt_value = x;
}
void sql_column_set_string(struct sql_column *p, const char *x)
{
  sql_check_nullptr(p);
  
  switch(p->type) {
    case sql_column_type_none:
    case sql_column_type_int:
    case sql_column_type_float:
      /* Nix weiter */
      break;
    case sql_column_type_str:
      /* Den Speicher frei geben */
      sql_column_set_none(p);
      break;
    default:
      /* Ungültiger typ! */
      sql_die_invalid_type(p);
  }; /* switch(p->type) */

  p->type = sql_column_type_str;
  p->str_value = sql_xstrdup(x);
}

void sql_column_add_sibbling(struct sql_column *p, struct sql_column *q, int pos)
{
  sql_check_nullptr(p);
  sql_check_nullptr(q);

  struct sql_column *it = p;
  for(; (NULL != it->prev) && (0 > pos); pos += 1, it = it->prev);
  for(; (NULL != it->next) && (0 < pos); pos -= 1, it = it->next);

  if(0 <= pos) {
    /* Neue Liste: it, q, it->next */
    struct sql_column *old_it_next = it->next;
    it->next = q;
    q->prev = it;
    q->next = old_it_next;

    if(NULL != old_it_next) {
      old_it_next->prev = q;
    } /* if(NULL != old_it_next) */
  } else {
    /* Neue Liste: it->prev, q, it */
    struct sql_column *old_it_prev = it->prev;
    it->prev = q;
    q->next = it;
    q->prev = old_it_prev;

    if(NULL != old_it_prev) {
      old_it_prev->next = q;
    } /* if(NULL != old_it_prev) */
  } /* if(0 <= pos) */
}

void sql_column_del_sibbling(struct sql_column *p)
{
  sql_check_nullptr(p);

  if(NULL != p->prev) {
    p->prev->next = p->next;
  } /* if(NULL != p->prev) */

  if(NULL != p->next) {
    p->next->prev = p->prev;
  } /* if(NULL != p->next) */
}

struct sql_column *sql_column_get_first_sibbling(struct sql_column *p)
{
  sql_check_nullptr(p);

  for(; NULL != p->prev; p = p->prev);

  return p;
}

struct sql_column *sql_column_get_last_sibbling(struct sql_column *p)
{
  sql_check_nullptr(p);

  for(; NULL != p->next; p = p->next);

  return p;
}
