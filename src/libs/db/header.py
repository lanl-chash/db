"""header

Copyright (c) 2015, Los Alamos National Security, LLC
All rights reserved.

Copyright (2015). Los Alamos National Security, LLC. This software was produced
under U.S. Government contract DE-AC52-06NA25396 for Los Alamos National
Laboratory (LANL), which is operated by Los Alamos National Security, LLC for
the U.S. Department of Energy. The U.S. Government has rights to use,
reproduce, and distribute this software. NEITHER THE GOVERNMENT NOR LOS ALAMOS
NATIONAL SECURITY, LLC MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY
LIABILITY FOR THE USE OF THIS SOFTWARE. If software is modified to produce
derivative works, such modified software should be clearly marked, so as not to
confuse it with the version available from LANL.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Read and parse db data headers.

Author: Curt Hash <chash@lanl.gov>

"""

import os


def read_header():
    """Reads a db header from stdin. """
    header = ''

    # Read one byte at a time to avoid reading more than the header.
    while True:
        char = os.read(0, 1)
        if char == '\n':
            break
        header += char

    if not header.startswith('#db\t'):
        raise ValueError('missing or malformed #db header')

    return header


def parse_header(header):
    """Parses a db header and returns a dictionary mapping column names to type
    and position.

    """
    columns = (header.rstrip()).split('\t')[1:]

    schema = {}
    for i, column in enumerate(columns):
        name, data_type = column.split(':')
        schema[name] = (i, data_type)

    return schema


def make_header(schema):
    """Returns a header from a schema like that returned by parse_header(). """
    items = schema.items()
    items.sort(key=lambda i: i[1][0])

    header = '#db\t' + '\t'.join(':'.join([name, datatype])
                                 for name, (_, datatype) in items)

    return header
