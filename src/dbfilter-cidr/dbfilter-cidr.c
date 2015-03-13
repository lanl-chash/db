// dbfilter-cidr
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
// Filter db data records with IP values based on include/exclude rules
// specified in a ACL file.
//
// Author: Curt Hash <chash@lanl.gov>

#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cdb.h"
#include "netacl.h"

#define BUFSIZE 16384
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct {
  netacl_t **acls;
  int size;
} acls_t;

// Apply the ACLs to the input data.
void
filter(acls_t *acls) {
  size_t bufsize = BUFSIZE;
  char *buf = malloc(bufsize);
  size_t offset = 0;

  while (fgets(buf + offset, bufsize - offset, stdin)) {
    size_t len = strlen(buf);

    if (buf[len - 1] == '\n') {
      offset = 0;
    } else {
      // Grow the line buffer.
      bufsize *= 2;
      buf = realloc(buf, bufsize);
      offset = len;
      continue;
    }

    size_t offset = 0;

    int i;
    int pass = 1;
    for (i = 0; i < acls->size; i++) {
      // Get the next token.
      char repl = 0;
      int j;
      for (j = offset; ; j++) {
        if (buf[j] == '\t' || buf[j] == '\n') {
          repl = buf[j];
          break;
        }
      }
      char *token = buf+offset;
      buf[j] = '\0';

      // Check.
      netacl_t *acl = acls->acls[i];
      if (acl && !netacl_pass(acl, token)) {
        pass = 0;
        break;
      }

      // Repair the string and move to the next token.
      buf[j] = repl;
      offset = j + 1;
    }

    if (pass) {
      printf("%s", buf);
    }
  }

#ifdef DEBUG
  free(buf);
#endif
}

// Prints an error message to stderr.
static void
perr(char *prog, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "%s error: ", basename(prog));
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

// Prints usage and exits.
void
usage(char *prog, int status) {
  printf("Usage: <data stream> | %s [OPTION]... [[COLUMN] [ACL PATH]]...\n\n",
         basename(prog));
  printf("  -h, --help                  Print this text and exit.\n");
  printf("\nACL PATH should contain a list of rules with the following "
         "syntax:\n\n");
  printf("  (+|-)CIDR\n\n");
  printf("'+' and '-' denote include and exclude rules, respectively.\n");
  printf("Blank lines and lines beginning with '#' are ignored.\n");

  exit(status);
}

int
main(int argc, char **argv) {
  // Parse options.
  static struct option long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
  };
  const char *options = "h";
  char opt;
  while ((opt = getopt_long(argc, argv, options, long_options, NULL)) != -1) {
    switch (opt) {
      case 'h':
        usage(argv[0], EXIT_SUCCESS);
        break;
      default:
        perr(argv[0], "unrecognized option '%c'\n", opt);
        usage(argv[0], EXIT_FAILURE);
        break;
    }
  }

  int nargs = argc - optind;
  if (nargs == 0 || nargs % 2 != 0) {
    // Expected at least one column name, ACL path pair.
    perr(argv[0], "missing required arguments\n");
    exit(EXIT_FAILURE);
  }

  // Parse the input #db header and replay it.
  char *header = read_header(stdin);
  schema_t schema;
  if (parse_header(header, &schema) != 0) {
    perr(argv[0], "error parsing #db header\n");
    exit(EXIT_FAILURE);
  }
  printf("%s\n", header);

  int i;

  // Initialize ACLs.
  acls_t acls = {calloc(sizeof (netacl_t *), schema.ncols), 0};
  for (i = optind; i < argc; i += 2) {
    char *name = argv[i];
    char *acl_path = argv[i+1];

    column_t *column = get_column(&schema, name);
    if (!column) {
      fprintf(stderr, "column '%s' is not present\n", name);
      exit(EXIT_FAILURE);
    }

    // Determine the maximum column index for which an ACL exists, so that we
    // can short circuit tokenization later.
    acls.size = MAX(column->index, acls.size);

    int ret;
    netacl_t *acl = malloc(sizeof (netacl_t));
    if ((ret = netacl_from_path(acl_path, acl)) != 0) {
      fprintf(stderr, "could not initialize ACL from path '%s'\n", acl_path);
      exit(EXIT_FAILURE);
    }

    acls.acls[column->index - 1] = acl;
  }

  // Apply the ACLs to the input data.
  filter(&acls);

#ifdef DEBUG
  // Free things.
  for (i = 0; i < acls.size; i++) {
    if (acls.acls[i]) {
      netacl_destroy(acls.acls[i]);
    }
  }
  free(acls.acls);
  free_schema(&schema);
  free(header);
#endif

  return 0;
}
