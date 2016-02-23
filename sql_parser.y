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
#include "sql_parser.h"
#include "sql_scanner.h"
#include <libgen.h>

/* Quelle: http://stackoverflow.com/a/32539752 */

void sqlerror(YYLTYPE *bloc, struct sql_context *ctx, yyscan_t scanner, char const *msg)
{
  sql_die("In line %i: %s!", sqlget_lineno(scanner), msg);
}

int main(int argc, char *argv[])
{ 
  yyscan_t scanner;
  int opt;
  struct sql_context sql = sql_context_init();
  sql.source_file = "stdin";
  while(-1 != (opt = getopt(argc, argv, "hqcdntf:"))) {
    switch(opt) {
      case 'h':
        printf("Usage: %s OPT FILE...\n", basename(argv[0]));
        printf("\n");
        printf("Converts sql-dump files into csv-files. All files will be\n");
        printf("parsed one by one. When `-' is given, `stdin' is read.\n");
        printf(" -h      Print this message and terminate.\n");
        printf(" -q      Only print errors.\n");
        #ifdef SQL_ZLIB
        printf(" -c      Compress resulting tables with zlib.\n");
        #endif /* SQL_ZLIB */
        printf(" -d      Ignore `drop table' statements.\n");
        printf(" -n      Insert column names as first line.\n");
        printf(" -t      Insert column types as comment.\n");
        printf(" -f FMT  Set print format for float values.\n");
        printf("\n");
        printf("Copyright 2016, rbnn\n");
        printf("Compiled: %s %s\n", __DATE__, __TIME__);
        return EXIT_SUCCESS;
        break;
      case 'q':
        sql_debug("Enabling silent mode.");
        sql_be_quiet = 1;
        break;
      case 'c':
        sql_debug("Enabling compression.");
        sql.compress = 1;
        break;
      case 'd':
        sql_debug("Ignoring drop-table statements.");
        sql.dont_drop = 1;
        break;
      case 'n':
        sql_debug("Enabling column names...");
        sql.add_header = 1;
        break;
      case 't':
        sql_debug("Enabling column types...");
        sql.add_types = 1;
        break;
      case 'f':
        sql_debug("Changing float format to `%s'...", optarg);
        sql.float_fmt = optarg;
        break;
      default:
        /* Programmabbruch, da die Option unbekannt war! */
        sql_die("Invalid option `-%c'!", opt);
    } /* switch(opt) */
  } /* while */
  
  for(; optind < argc; optind += 1) {
    FILE *f_in = NULL;
    char *fname = argv[optind];
    struct sql_context ctx = sql;
    sql_debug("Reading file `%s'...", fname);

    if(0 == strcmp("-", fname)) {
      f_in = stdin;
      ctx.source_file = "stdin";
    } else {
      f_in = fopen(fname, "r");
      ctx.source_file = fname;
    } /* if(0 == strcmp ... ) */

    sql_check_nullptr(f_in);

    sqllex_init(&scanner);
    sqlset_in(f_in, scanner);
    if(0 == sqlparse(&ctx, scanner)) {
      /* Parsen war erfolgreich */
      sql_debug("Conversion to csv was successful.");
    } /* if(0 == sqlparse ... ) */

    sqllex_destroy(scanner);
    sql_context_destroy(&ctx);

    if(0 != strcmp("-", fname)) {
      fclose(f_in);
    } /* if(0 != strcmp ... ) */
  } /* for... */
  sql_context_destroy(&sql);
  return 0;
}

%}
%code requires {
#include <obstack.h>
#include "sql.h"
}
%output "sql_parser.c"
%defines "sql_parser.h"
%locations
%error-verbose
%define api.pure full
%define api.prefix sql
%lex-param    {void *scanner}
%parse-param  {struct sql_context *ctx}
%parse-param  {void *scanner}
%start sequence_of_statements

%union {
  long long   int_value;
  long double flt_value;
  char       *str_value;
  struct obstack os;
  struct sql_column *new_column;
  struct sql_table *new_table;
}

%token <int_value> INT
%token <flt_value> FLOAT
%token <str_value> STRING ID
%token LPAREN RPAREN SEMICOLON COMMA SETTO
%token KW_CREATE KW_DEFAULT KW_DROP KW_EXISTS
%token KW_IF KW_INSERT KW_INTO KW_KEY KW_LOCK
%token KW_NOT KW_NULL KW_PRIMARY KW_TABLE KW_TABLES 
%token KW_UNLOCK KW_VALUES KW_WRITE

%token KW_INT KW_UNSIGNED

%type <new_table> create_table_statement
%type <new_column> create_table_columns_statement create_table_column_statement insert_into_values_columns
%%
sequence_of_statements:
  {
    sql_debug("Found empty statement...");
    /* Nix weiter */
  }
  |
  sequence_of_statements statement
  {
    sql_debug("Found new statement...");
    /* Nix weiter */
  }
  ;

statement:
  SEMICOLON
  {
    sql_debug("Found a stray semicolon...");
    /* Nix weiter */
  }
  |
  drop_table_statement SEMICOLON
  {
    sql_debug("Found drop-table statement...");
    /* Nix weiter */

  }
  |
  create_table_statement SEMICOLON
  {
    sql_debug("Found create-table statement...");
    sql_context_add_table(ctx, $1);
  }
  |
  lock_table_statement SEMICOLON
  {
    sql_debug("Found lock-table statement...");
    /* Nix weiter */
  }
  |
  unlock_table_statement SEMICOLON
  {
    sql_debug("Found unlock-table statement...");
    /* Nix weiter */
  }
  |
  insert_into_statement SEMICOLON
  {
    sql_debug("Found insert statement...");
    /* Nix weiter */
  }
  ;

drop_table_statement:
  KW_DROP KW_TABLE KW_IF KW_EXISTS STRING
  {
    /* Tabellendaten sollen nicht verworfen werden */
    sql_warning("Ignoring drop statement for table `%s'...", $5);
    sql_xfree($5);
  }
  ;

create_table_statement:
  KW_CREATE KW_TABLE STRING LPAREN create_table_columns_statement RPAREN
  {
    $$ = sql_table_new();
    sql_table_set_name($$, $3);
    $$->first_column = sql_column_get_first_sibbling($5);
    $$->last_column = sql_column_get_last_sibbling($5);
    sql_xfree($3);
  }
  |
  create_table_statement KW_DEFAULT
  {
    sql_warning("Ignoring `default' keyword for table!");
  }
  |
  create_table_statement ID SETTO ID
  {
    sql_warning("Ignoring `%s=%s' for table!", $2, $4);
    sql_xfree($4);
    sql_xfree($2);
  }
  |
  create_table_statement ID SETTO INT
  {
    sql_warning("Ignoring `%s=%lli' for table!", $2, $4);
    sql_xfree($2);
  }
  ;

create_table_columns_statement:
  create_table_column_statement
  {
    $$ = $1;
  }
  |
  create_table_columns_statement COMMA create_table_column_statement
  {
    sql_column_add_sibbling(sql_column_get_last_sibbling($1), $3, 1);
    $$ = sql_column_get_last_sibbling($3);
  }
  |
  create_table_columns_statement COMMA create_table_primary_key_statement
  {
    sql_warning("Ignorig primary key definition!");
  }
  ;

create_table_column_statement:
  STRING KW_INT LPAREN INT RPAREN
  {
    $$ = sql_column_new();
    sql_column_set_name($$, $1);
    sql_column_set_int($$, 0L);
    sql_xfree($1);
  }
  |
  create_table_column_statement KW_UNSIGNED
  {
    sql_warning("Ignoring `unsigned' for column!");
  }
  |
  create_table_column_statement KW_NOT KW_NULL
  {
    sql_warning("Ignoring `not null' for column!");
  }
  |
  create_table_column_statement KW_DEFAULT KW_NULL
  {
    sql_warning("Ignoring `default null' for column!");
  }
  |
  create_table_column_statement ID
  {
    sql_warning("Ignoring `%s' for column!", $2);
    sql_xfree($2);
  }
  ;

create_table_primary_key_statement:
  KW_PRIMARY KW_KEY LPAREN create_table_primary_key_column_list RPAREN
  ;

create_table_primary_key_column_list:
  STRING
  {
    sql_xfree($1);
  }
  |
  create_table_primary_key_column_list COMMA STRING
  {
    sql_xfree($3);
  }
  ;

lock_table_statement:
  KW_LOCK KW_TABLES STRING KW_WRITE
  {
    sql_debug("Locking table `%s'...", $3);
    sql_context_lock_table(ctx, $3);
    sql_xfree($3);
  }
  ;

unlock_table_statement:
  KW_UNLOCK KW_TABLES
  {
    sql_debug("Releasing current table...");
    sql_context_unlock_table(ctx);
  }
  ;

insert_into_statement:
  KW_INSERT KW_INTO STRING KW_VALUES insert_into_values_rows
  {
    if(NULL == ctx->current_table) {
      /* Programmabbruch, da die Tabelle nicht gewählt wurde! */
      sql_die("Table `%s' must be locked prior to use!", $3);
    } /* if(NULL == current_table) */
    if(0 != strcmp(ctx->current_table->name, $3)) {
      /* Programmabbruch, da die falsche Tabelle gewählt wurde! */
      sql_die("Invalid write detected! Table `%s' must be locked prior to use! (currently locked: `%s')", $3, ctx->current_table->name);
    } /* if(0 != strcmp ... ) */
    sql_xfree($3);
  }
  ;

insert_into_values_rows:
  insert_into_values_row
  {
    sql_check_nullptr(ctx->current_table);
    sql_debug("Writing row into table `%s'...", ctx->current_table->name);
    sql_context_write_current_row(ctx);
  }
  |
  insert_into_values_rows COMMA insert_into_values_row
  {
    sql_debug("Writing row into table `%s'...", ctx->current_table->name);
    sql_context_write_current_row(ctx);
  }
  ;

insert_into_values_row:
  LPAREN insert_into_values_columns RPAREN
  ;

insert_into_values_columns:
  INT
  {
    struct sql_column *col = sql_context_get_current_column(ctx);
    sql_context_next_column(ctx);
    sql_column_set_int(col, $1);
  }
  |
  FLOAT
  {
    struct sql_column *col = sql_context_get_current_column(ctx);
    sql_context_next_column(ctx);
    sql_column_set_float(col, $1);
  }
  |
  insert_into_values_columns COMMA INT
  {
    struct sql_column *col = sql_context_get_current_column(ctx);
    sql_context_next_column(ctx);
    sql_column_set_int(col, $3);
  }
  ;
%%
