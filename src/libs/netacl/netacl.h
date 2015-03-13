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

#ifndef NETACL_H
#define NETACL_H

#include "stdio.h"

#include "libcidr.h"

#define INITIAL_VECTOR_SIZE 32
#define BUFSIZE 16384

enum netacl_err {
  ERR_SYNTAX = 256
};

// Vector of CIDRs.
typedef struct {
  CIDR **cidrs;
  uint32_t size;
  uint32_t capacity;
} cidr_vector_t;

// ACL.
typedef struct {
  cidr_vector_t include;
  cidr_vector_t exclude;
} netacl_t;

// Load from file path.
int
netacl_from_path(const char *, netacl_t *);

// Load from file descriptor.
int
netacl_from_fd(int, netacl_t *);

// Load from open file.
int
netacl_from_file(FILE *, netacl_t *);

// Test an IP against the ACL.
int
netacl_pass(const netacl_t *, const char *addr);

// Free an ACL.
void
netacl_destroy(netacl_t *);

#endif // NETACL_H
