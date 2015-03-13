db
==

db is a set of utilities for munging and querying data files in TSV
(tab-separated value) or JSON format. The utilities are designed such that
complex data processing tasks can be accomplished using idomatic constructs
such as SQL queries and shell pipes.

Install
-------

You can either build and install an Ubuntu .deb package or install from source
using the `make` command. Both methods have been tested on Ubuntu 14.04 LTS
x64.

1. Download and extract the source tree.
2. Install libpcap and libpcap development files (libpcap-dev on Ubuntu).

### Ubuntu Package

Build and install an Ubuntu .deb package:

```
$ make deb
$ sudo dpkg -i ../db_*.deb
$ sudo apt-get -f install
```

To remove the package:

```
$ sudo apt-get remove db
```

### Source

Install Python 2.7, python-dateutil, python-pyparsing, python-configobj,
python-flufl.lock, sqlite3 and nodejs. Then:

```
$ make
$ sudo make install
```

To remove all files and directories created by `make install`:

```
$ sudo make uninstall
```

File Formats
------------

The db utilities support two data file formats: TSV and JSON.

### TSV

A header followed by TSV data records (one per line). Values may not contain
tabs.

#### Header Format

```
#db[TAB][COLNAME]:[COLTYPE][[TAB][COLNAME]:[COLTYPE]...]
```

`COLNAME` should be a short, meaningful, alphanumeric string.

`COLTYPE` must be one of `{str, int, real}`.

Example:

```
#db	src:str	dst:str
```

### JSON

JSON records, one per line. The records need not have a fixed schema.

Manifest
--------

### Utilities

The following utilities are provided in the db package.

| File | Purpose |
| ---- | ------- |
| catmux | Used by dbcat and jsoncat (don't use directly) |
| db2json | Convert db data to JSON |
| db2sqlite | Import db data into an sqlite3 database |
| dbcat | Concatenate or multiplex db data files |
| dbfilter-cidr | Filter records using column-based include/exclude CIDR rules |
| dbsort | Sort records by column name using \*nix sort |
| dbsplit | Split/partition a stream into multiple output streams |
| dbsqawk | Query db records using SQL compiled to awk |
| dbstrip | Strip the #db header |
| jsoncat | Concatenate or multiplex JSON data files |
| jsonsort | Sort records by field name using \*nix sort |
| jsonsplit | Split/partition a JSON stream into multiple output streams |
| jsonsql | Query JSON records using SQL compiled to JavaScript |
| mux | Used by dbcat and jsoncat (don't use directly) |
| timefind | Program for identifying data files overlapping a time period |

### Libraries

The following libraries are included in the source tree.

| Library | Purpose |
| ------- | ------- |
| cdb | C functions for reading/parsing #db headers |
| db | Python functions for reading/parsing #db headers |
| godb | Go functions for reading/parsing #db headers |
| libcidr | C library for dealing with CIDRs |
| netacl | C library for IP filtering |

License
-------

Copyright &copy; 2014 Los Alamos National Security, LLC.

This software is licensed under the MIT License. See the LICENSE file for more
information.
