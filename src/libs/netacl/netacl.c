// netacl
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
// CIDR ACL built on libcidr.
//
// Author: Curt Hash <chash@lanl.gov>

#include "arpa/inet.h"
#include "errno.h"
#include "stdlib.h"
#include "string.h"

#include "netacl.h"

// Initializes a CIDR vector.
static inline void
cidr_vector_init(cidr_vector_t *v) {
  v->cidrs = malloc(sizeof (CIDR *) * INITIAL_VECTOR_SIZE);
  v->size = 0;
  v->capacity = INITIAL_VECTOR_SIZE;
}

// Pushes a CIDR onto a vector.
static inline void
cidr_vector_push(cidr_vector_t *v, CIDR *cidr) {
  if (v->size == v->capacity) {
    v->capacity *= 2;
    v->cidrs = realloc(v->cidrs, sizeof (CIDR *) * v->capacity);
  }

  v->cidrs[v->size++] = cidr;
}

// Destroys a CIDR vector.
static inline void
cidr_vector_free(cidr_vector_t *v) {
  int i;
  for (i = 0; i < v->size; i++) {
    cidr_free(v->cidrs[i]);
  }

  free(v->cidrs);
}

// Initializes an ACL.
static inline void
netacl_init(netacl_t *acl) {
  cidr_vector_init(&acl->include);
  cidr_vector_init(&acl->exclude);
}

// Frees an ACL.
void
netacl_destroy(netacl_t *acl) {
  cidr_vector_free(&acl->include);
  cidr_vector_free(&acl->exclude);
}

// Loads an ACL from a file path.
int
netacl_from_path(const char *path, netacl_t *acl) {
  FILE *fp = fopen(path, "r");
  if (!fp) {
#ifdef DEBUG
    perror("fopen");
#endif
    return errno;
  }

  return netacl_from_file(fp, acl);
}

// Loads an ACL from a file descriptor.
int
netacl_from_fd(int fd, netacl_t *acl) {
  FILE *fp = fdopen(fd, "r");
  if (!fp) {
#ifdef DEBUG
    perror("fdopen");
#endif
    return errno;
  }

  return netacl_from_file(fp, acl);
}

// Loads an ACL from an open file.
//
// Rule syntax:
// [+|-][CIDR]
//
// A '+' indicates an include rule; '-' indicates an exclude rule.
//
// ACL files contain 1 rule per line.
//
// Blank lines and lines beginning with '#' are ignored.
int
netacl_from_file(FILE *fp, netacl_t *acl) {
  netacl_init(acl);

  size_t bufsize = BUFSIZE;
  char *buffer = malloc(bufsize);
  size_t offset = 0;

  // Read one line at a time.
  int i = 0;
  while (fgets(buffer + offset, bufsize - offset, fp)) {
    if (buffer[strlen(buffer) - 1] == '\n') {
      offset = 0;
    } else {
      bufsize *= 2;
      buffer = realloc(buffer, bufsize);
      offset = strlen(buffer);
      continue;
    }

    i++;

    // Strip the new line.
    buffer[strlen(buffer) - 1] = '\0';

    if (strlen(buffer) == 0 || buffer[0] == '#') {
      // Ignore blank lines and comments.
      continue;
    }

    char rule_type = buffer[0];
    if (rule_type != '+' && rule_type != '-') {
#ifdef DEBUG
      fprintf(stderr, "netacl error: rule type syntax error, line %d\n", i);
#endif
      return ERR_SYNTAX;
    }

    // Parse the CIDR.
    CIDR *cidr = cidr_from_str(buffer+1);
    if (!cidr) {
#ifdef DEBUG
      fprintf(stderr, "netacl error: CIDR syntax error, line %d\n", i);
#endif
      return ERR_SYNTAX;
    }

    // Add the CIDR to the include or exclude vector.
    if (rule_type == '+') {
      cidr_vector_push(&acl->include, cidr);
#ifdef DEBUG
      fprintf(stderr, "netacl: added include rule %s\n", buffer);
#endif
    } else {
#ifdef DEBUG
      fprintf(stderr, "netacl: added exclude rule %s\n", buffer);
#endif
      cidr_vector_push(&acl->exclude, cidr);
    }
  }

  free(buffer);
  fclose(fp);

  return 0;
}

// Returns 1 if:
//  (a) the CIDR belongs to one of the networks in the include vector or there
//      are no include rules; and
//  (b) the CIDR does not belong to one of the networks in the exclude vector
//      or there are no exclude rules.
//
// Returns 0 otherwise.
inline int
netacl_pass(const netacl_t *acl, const char *addr) {
  int i;
  int pass = 1;

  CIDR *cidr = cidr_from_str(addr);

  if (acl->include.size) {
    pass = 0;
    for (i = 0; i < acl->include.size; i++) {
      if (cidr_contains(acl->include.cidrs[i], cidr) == 0) {
        // Matched an include rule.
        pass = 1;
        break;
      }
    }
  }

  // Pass will be 0 if no include rules matched.
  if (pass) {
    for (i = 0; i < acl->exclude.size; i++) {
      if (cidr_contains(acl->exclude.cidrs[i], cidr) == 0) {
        // Matched an exclude rule.
        pass = 0;
        break;
      }
    }
  }

  cidr_free(cidr);

  return pass;
}
