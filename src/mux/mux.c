/**
 * mux
 *
 * Copyright (c) 2015, Los Alamos National Security, LLC
 * All rights reserved.
 *
 * Copyright (2015). Los Alamos National Security, LLC. This software was
 * produced under U.S. Government contract DE-AC52-06NA25396 for Los Alamos
 * National Laboratory (LANL), which is operated by Los Alamos National
 * Security, LLC for the U.S. Department of Energy. The U.S. Government has
 * rights to use, reproduce, and distribute this software. NEITHER THE
 * GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY, EXPRESS
 * OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE. If
 * software is modified to produce derivative works, such modified software
 * should be clearly marked, so as not to confuse it with the version available
 * from LANL.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Multiplex lines from multiple inputs to stdout.
 *
 * Author: Curt Hash <chash@lanl.gov>
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#define READ_COUNT 16384

typedef enum {
  FMT_DB,
  FMT_JSON
} format_t;

typedef struct {
  char **inputs;
  int input_count;
  format_t format;
} options_t;

typedef struct {
  int fd;
  char *buffer;
  size_t size;
  size_t offset;
  char closed;
} fd_t;

/**
 * Reads and, if specified, prints the db data header.
 */
void
read_header(int fd, char print) {
  int ret;
  char c = '\0';

  while (c != '\n') {
    ret = read(fd, &c, 1);
    if (ret == -1) {
      perror("read");
      exit(errno);
    } else if (ret == 0) {
      fprintf(stderr, "EOF before end of header\n");
      exit(1);
    }

    if (print) {
      printf("%c", c);
    }

    if (c == '\n') {
      break;
    }
  }
}

int
mux(options_t *options) {
  int i;
  int j;
  int fd_open_count = 0;
  fd_t *fds = NULL;
  fd_set readfds;
  int nfds;
  fd_t *f;
  int ret;
  int length;
  int rest;
  char print = 1;

  /* Initialize fds. */
  fds = malloc(sizeof (fd_t) * options->input_count);
  for (i=0; i<options->input_count; i++) {
    f = &fds[i];

    if (access(options->inputs[i], R_OK) != 0) {
      perror("access");
      return errno;
    }

    f->fd = open(options->inputs[i], O_RDONLY);
    if (f->fd == -1) {
      perror("open");
      return errno;
    }

    if (options->format == FMT_DB) {
      /* Read the header. Print only the first. */
      read_header(f->fd, print);
      print = 0;
    }

    fd_open_count++;

    f->buffer = malloc(READ_COUNT);
    f->size = READ_COUNT;
    f->offset = 0;
    f->closed = 0;
  }

  while (fd_open_count) {
    FD_ZERO(&readfds);
    nfds = 0;

    /* Add open fds to the select set. */
    for (i=0; i<options->input_count; i++) {
      f = &fds[i];
      if (!f->closed) {
        FD_SET(f->fd, &readfds);

        if (f->fd+1 > nfds) {
          nfds = f->fd+1;
        }

        fcntl(f->fd, F_SETFL, O_NONBLOCK);
      }
    }

    if (select(nfds, &readfds, NULL, NULL, NULL) == -1) {
      perror("select");
      return errno;
    }

    /* Read from ready fds. */
    for (i=0; i<options->input_count; i++) {
      f = &fds[i];

      if (f->closed || !FD_ISSET(f->fd, &readfds)) {
        continue;
      }

      ret = read(f->fd, f->buffer + f->offset, f->size - f->offset);
      if (ret == -1) {
        perror("read");
        return errno;
      } else if (ret == 0) {
        /* EOF. */
        close(f->fd);
        f->closed = 1;
        fd_open_count--;
        continue;
      }

      /* Search for the last newline. */
      for (j=ret-1; j>=0; j--) {
        if (f->buffer[f->offset+j] == '\n') {
          /* Newline found. Output buffered lines. */
          length = f->offset+j+1;
          fwrite(f->buffer, 1, length, stdout);

          /* Shift the rest of the buffer and update the offset. */
          rest = ret-j-1;
          memmove(f->buffer, f->buffer+length, rest);
          f->offset = rest;

          break;
        }
      }

      if (j < 0) {
        /* No newline found in the buffer. */
        f->offset += ret;
      }

      /* Grow the buffer if there isn't enough room for another read(). */
      if (f->size - f->offset < READ_COUNT) {
        f->size *= 2;
        f->buffer = realloc(f->buffer, f->size);
      }
    }
  }

  fflush(stdout);

  return 0;
}

void
usage(const char *prog, int status) {
  printf("Usage: %s [OPTION]... FORMAT INPUT [INPUT]...\n\n", prog);
  printf("Format must be one of [db|json].\n\n");
  printf("  -h, --help             Show this text and exit.\n");

  exit(status);
}

int
main(int argc, char **argv) {
  options_t options;
  char *format;

  /* Parse options. */
  static struct option longopts[] = {
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
  };
  const char *optstring = "h";
  char opt;
  while ((opt = getopt_long(argc, argv, optstring, longopts, NULL)) != -1) {
    switch (opt) {
      case 'h':
        usage(argv[0], 0);
        break;
      default:
        fprintf(stderr, "unrecognized option '%c'\n", opt);
        usage(argv[0], 1);
        break;
    }
  }

  if (argc - optind == 0) {
    fprintf(stderr, "missing FORMAT and INPUT...\n");
    usage(argv[0], 1);
  }

  format = *(argv + optind);
  if (strcmp(format, "db") == 0) {
    options.format = FMT_DB;
  } else if (strcmp(format, "json") == 0) {
    options.format = FMT_JSON;
  } else {
    fprintf(stderr, "invalid input format '%s'\n", format);
    exit(1);
  }

  options.input_count = argc - optind - 1;
  if (options.input_count == 0) {
    fprintf(stderr, "missing INPUT...\n");
    usage(argv[0], 1);
  }

  options.inputs = argv + optind + 1;

  exit(mux(&options));
}
