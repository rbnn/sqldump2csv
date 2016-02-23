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
%{
#include <obstack.h>
#include "sql.h"

/* Quelle: http://stackoverflow.com/a/32539752 */

#define YYSTYPE SQLSTYPE
#define YYLTYPE SQLLTYPE
#include "sql_parser.h"

#define obstack_chunk_alloc sql_xmalloc
#define obstack_chunk_free  sql_xfree
%}

%option yylineno
%option reentrant bison-bridge bison-locations
%option header-file="sql_scanner.h"
%option outfile="sql_scanner.c"
%option prefix="sql"
%option noyywrap noinput

%x INCOMMENT INLCOMMENT INSTRING INSEMICOLON
  /* %option nodefault */
number      [[:digit:]]+
hex_number  [[:xdigit:]]+
float       ({number}[.]{number}?|[.]{number})
name        [[:alpha:]_][[:alnum:]_]*
space       [[:space:]]
%%
  /* -- Keywords --
   * -------------- */
(?i:create)       { return(KW_CREATE);  }
(?i:default)      { return(KW_DEFAULT); }
(?i:drop)         { return(KW_DROP);    }
(?i:exists)       { return(KW_EXISTS);  }
(?i:if)           { return(KW_IF);      }
(?i:insert)       { return(KW_INSERT);  }
(?i:into)         { return(KW_INTO);    }
(?i:key)          { return(KW_KEY);     }
(?i:lock)         { return(KW_LOCK);    }
(?i:not)          { return(KW_NOT);     }
(?i:null)         { return(KW_NULL);    }
(?i:primary)      { return(KW_PRIMARY); }
(?i:table)        { return(KW_TABLE);   }
(?i:tables)       { return(KW_TABLES);  }
(?i:unlock)       { return(KW_UNLOCK);  }
(?i:values)       { return(KW_VALUES);  }
(?i:write)        { return(KW_WRITE);   }

  /* -- Types --
     ----------- */
(?i:(big)?int)    { return(KW_INT);     }
(?i:unsigned)     { return(KW_UNSIGNED);}

  /* -- Kommentare --
   * ---------------- */
<INITIAL>"/*"   { BEGIN(INCOMMENT);  }
<INCOMMENT>.|\n { /* Nix weiter */   }
<INCOMMENT>"*/" { BEGIN(INITIAL);    }
<INITIAL>--     { BEGIN(INLCOMMENT); }
<INLCOMMENT>.   { /* Nix weiter */   }
<INLCOMMENT>\n  { BEGIN(INITIAL);    }

  /* -- String/Text --
   * ----------------- */
<INITIAL>`     {
  BEGIN(INSTRING);
  obstack_init(&yylval->os);
  }

<INSTRING>`     {
  BEGIN(INITIAL);
  obstack_1grow(&yylval->os, '\0');
  const char *final_string = (const char*)obstack_finish(&yylval->os);
  char *str = sql_xstrdup(final_string);
  obstack_free(&yylval->os, NULL);
  yylval->str_value = str;
  return(STRING);
  }

<INSTRING>.|\n     {
  /* Das neue Zeichen hinzufügen */
  obstack_1grow(&yylval->os, *yytext);
  }

  /* -- Integer (Base 10) --
   * ----------------------- */
[+-]?{number}     {
  /* Integer als Dezimalzahl */
  yylval->int_value = atoll(yytext);
  return(INT);
  }

  /* -- Integer (Base 16) --
   * ----------------------- */
0x{hex_number}  {
  /* Integer als Hexagesimalzahl */
  yylval->int_value = strtoll(yytext, NULL, 16);
  return(INT);
  }

  /* -- Float --
   * ----------- */
[+-]?{float} {
  yylval->flt_value = atof(yytext);
  return(FLOAT);
  }

  /* -- Float (mit Exponent) --
   * -------------------------- */
[+-]?({float}|{number})[Ee][+-]?{number} {
  yylval->flt_value = atof(yytext);
  return(FLOAT);
  }

  /* -- Float (Unendlich) --
   * ----------------------- */
[+-]?"inf"        {
  yylval->flt_value = atof(yytext);
  return(FLOAT);
  }

  /* -- Variablen --
   * --------------- */
{name}            {
  yylval->str_value = sql_xstrdup(yytext);
  return(ID);
  }

  /* -- Symbole zur Gruppierung --
   * ----------------------------- */
"("               { return(LPAREN); }
")"               { return(RPAREN); }

  /* -- Semikolon --
   * --------------- */
<INITIAL>";"          { BEGIN(INSEMICOLON); }
<INSEMICOLON>";"      { /* Nix weiter */    }
<INSEMICOLON>{space}  { /* Nix weiter */    }
<INSEMICOLON><<EOF>>  { BEGIN(INITIAL); return(SEMICOLON); }
<INSEMICOLON>.        { BEGIN(INITIAL); unput(*yytext); return(SEMICOLON); }

  /* -- Sonstige Symbole --
   * ---------------------- */
","               { return(COMMA); }
"="               { return(SETTO); }

  /* -- Leerzeichen werden ignoriert --
   * ---------------------------------- */
{space}           { /* Nix weiter */ }

  /* Fehler erzeugen, wenn eine Eingabe nicht verarbeitet werden konnte! */
  /* .                 {
  //   cfg_error("Unexpected `%c' in line %i!", *yytext, yylineno);
  //   return(character);
     } */
%%
