// tf_pcap
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
// Output the minimum and maximum packet time of the PCAP file specified using
// the first argument.
//
// Author: Curt Hash <chash@lanl.gov>

#include <pcap.h>

double
timeval2epoch(struct timeval *t) {
  return (double)t->tv_sec + ((double)t->tv_usec / 1000000);
}

int
main(int argc, char **argv) {
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *handle = pcap_open_offline(argv[1], errbuf);
  if (!handle) {
    fprintf(stderr, "pcap_open_offline('%s'): %s\n", argv[1], errbuf);
    return 1;
  }

  struct pcap_pkthdr pcap;
  pcap_next(handle, &pcap);

  struct timeval min = pcap.ts;

  while (pcap_next(handle, &pcap)) {}
  pcap_close(handle);

  struct timeval max = pcap.ts;

  printf("%s|%f|%f\n", argv[1], timeval2epoch(&min), timeval2epoch(&max));

  return 0;
}
