// cdb
//
// Copyright (c) 2015, Los Alamos National Security, LLC
// All rights reserved.
//
// Copyright (2015). Los Alamos National Security, LLC. This software was
// produced under U.S. Government contract DE-AC52-06NA25396 for Los Alamos
// National Laboratory (LANL), which is operated by Los Alamos National
// Security, LLC for the U.S. Department of Energy. The U.S. Government has
// rights to use, reproduce, and distribute this software. NEITHER THE
// GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY, EXPRESS
// OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE. If
// software is modified to produce derivative works, such modified software
// should be clearly marked, so as not to confuse it with the version available
// from LANL.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
// Read and parse db headers.
//
// Author: Curt Hash <chash@lanl.gov>

#include "cdb.h"

char *
read_header(FILE *fp) {
  int fd = fileno(fp);
  char *header = malloc(MAX_HEADER);

  int i = 0;
  while (i < MAX_HEADER - 1) {
    int bytes = read(fd, header + i, 1);
    if (bytes == -1 || bytes == EOF) {
      fprintf(stderr, "read_header error\n");
      return NULL;
    }

    if (header[i] == '\n') {
      break;
    }
    i++;
  }

  header[i] = '\0';

  return header;
}

int
parse_header(const char *header, schema_t *schema) {
  int ret = 0;

  char *copy = malloc(strlen(header)+1);
  strcpy(copy, header);

  schema->head = schema->tail = NULL;
  schema->ncols = 0;

  const char *delim = "\t\r\n";
  char *saveptr1;
  char *token = strtok_r(copy, delim, &saveptr1);
  if (strcmp(token, "#db") != 0) {
    fprintf(stderr, "header missing #db magic\n");
    ret = 1;
    goto cleanup;
  }

  while ((token = strtok_r(NULL, delim, &saveptr1))) {
    column_t *column = malloc(sizeof (column_t));
    column->flink = NULL;

    char *saveptr2;
    char *subtoken = strtok_r(token, ":", &saveptr2);
    column->name = malloc(strlen(subtoken)+1);
    strcpy(column->name, subtoken);

    subtoken = strtok_r(NULL, "", &saveptr2);
    if (!subtoken) {
      fprintf(stderr, "db header syntax error, column %d\n", schema->ncols+1);
      ret = 1;
      goto cleanup;
    }
    column->type = malloc(strlen(subtoken)+1);
    strcpy(column->type, subtoken);

    column->index = schema->ncols + 1;

    if (schema->ncols) {
      schema->tail->flink = column;
      schema->tail = column;
    } else {
      schema->head = schema->tail = column;
    }

    schema->ncols++;
  }

cleanup:
  free(copy);

  return ret;
}

column_t *
get_column(const schema_t *schema, const char *name) {
  column_t *column = schema->head;

  while (column) {
    if (strcmp(column->name, name) == 0) {
      return column;
    }

    column = column->flink;
  }

  return NULL;
}

void
free_schema(schema_t *schema) {
  column_t *column = schema->head;

  while (column) {
    column_t *del = column;
    column = column->flink;

    free(del->name);
    free(del->type);
    free(del);
  }

  schema->head = schema->tail = NULL;
  schema->ncols = 0;
}
