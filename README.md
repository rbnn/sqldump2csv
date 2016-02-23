# sqldump2csv

The command line tool **sqldump2csv** converts MySQL-dump files into csv-files.
The input is split into one file per table that can be gzipped optionally.

## Installation

If you have *GNU-bison*(2.7.12), *flex*(2.5.39) and *zlib*(1.2.8) installed, just type
```
make
```

## Notice

So far, the program is limited to **integer columns**.
