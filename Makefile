CFLAGS+=-Wall -Werror
LDFLAGS=-lz
WITH_ZLIB=1

ifeq ($(WITH_DEBUG),1)
	CFLAGS+=-g3 -O0 -DSQL_DEBUG
endif

ifeq ($(WITH_ZLIB),1)
	CFLAGS+=-DSQL_ZLIB
endif

sqldump2csv: sql_scanner.o sql_parser.o sql_column.o sql_context.o sql_table.o sql_utils.o
	$(CC) -o $@ -Wl,--start-group $? -Wl,--end-group $(LDFLAGS)

sql_parser.c:
	bison sql_parser.y

sql_scanner.c: sql_parser.c
	flex sql_scanner.l
