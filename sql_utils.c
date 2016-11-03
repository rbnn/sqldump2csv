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
#include <string.h>

int sql_be_quiet = 0;

void *sql_xmalloc(size_t s)
{
  void *ptr = NULL;
  if((0 != s) && (NULL == (ptr = malloc(s)))) {
    /* Programmabbruch, da der Speicher nicht allokiert werden konnte! */
    sql_die("Could not allocate %zu bytes!", s);
  } /* if ...malloc ... */
  return ptr;
}

void sql_xfree(void *p)
{
  if(NULL != p) {
    free(p);
  } /* if(NULL != p) */
}

char *sql_xstrdup(const char *s)
{
  if(NULL == s) {
    /* Nix weiter */
    return NULL;
  } else {
    const size_t l = strlen(s);
    char *cpy = sql_xmalloc(1 + l);
    memcpy(cpy, s, l);
    *(cpy + l) = 0;
    return cpy;
  } /* if(NULL == s) */
}
