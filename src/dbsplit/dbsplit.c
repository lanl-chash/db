// dbsplit
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
// Splits a db data stream on stdin into multiple output streams.
//
// Author: Curt Hash <chash@lanl.gov>

#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cdb.h"
#include "uthash.h"
#include "xxhash.h"

#define BUFSIZE 16384

typedef struct {
  uint32_t parts;
  char **key;
  size_t keylen;
  char set;
  const char *prefix;
  char **outputs;
  char uniq;
} options_t;

typedef struct {
  char *key;
  FILE *fp;
  UT_hash_handle hh;
} fp_map_t;

// Integer compare for qsort().
int
cmp_int(const void *x, const void *y) {
  return *(const int *)x - *(const int *)y;
}

// String compare for qsort().
int
cmp_str(const void *a, const void *b) {
  return strcmp(*(const char **)a, *(const char **)b);
}

// Validates the key against the schema and returns the indexes of the columns
// in sorted order.
int *
get_indexes(char **key, size_t keylen, schema_t *schema) {
  int *indexes = malloc(sizeof (int) * keylen);
  int i;
  for (i=0; i<keylen; i++) {
    // Validate key column membership in the schema.
    column_t *column = get_column(schema, key[i]);
    if (!column) {
      fprintf(stderr, "invalid key column '%s'\n", key[i]);
      exit(1);
    }

    indexes[i] = column->index;
  }

  // Sort the key column indexes in the order that they will be read.
  qsort(indexes, keylen, sizeof (int), cmp_int);

  return indexes;
}

// Opens output files.
FILE **
open_output_files(options_t *options, const char *header) {
  FILE **fps = malloc(sizeof (FILE *) * options->parts);
  int i;
  for (i=0; i<options->parts; i++) {
    if (options->outputs) {
      // Open using the user-supplied path.
      fps[i] = fopen(options->outputs[i], "w");
    } else {
      // Open files using the user-supplied prefix and partition number.
      char name[256];
      snprintf(name, sizeof (name), "%s.%d", options->prefix, i);

      fps[i] = fopen(name, "w");
    }

    if (!fps[i]) {
      perror("could not open output file");
      exit(-errno);
    }

    fprintf(fps[i], "%s\n", header);
  }

  return fps;
}

// Returns the file pointer associated with the given key, creating it if
// necessary.
FILE *
get_uniq_fp(fp_map_t **uniq_fp_map, char *key, const char *prefix, int *part,
            const char *header, char is_const) {
  fp_map_t *item = NULL;
  HASH_FIND_STR(*uniq_fp_map, key, item);
  if (item) {
    // Existing key.
    if (!is_const) {
      free(key);
    }

    return item->fp;
  }

  // New key. Open an output file using the user-supplied prefix and partition
  // number.
  char name[256];
  snprintf(name, sizeof (name), "%s.%d", prefix, (*part)++);

  FILE *fp = fopen(name, "w");
  if (!fp) {
    perror("could not open output file");
    exit(-errno);
  }

  fprintf(fp, "%s\n", header);

  item = malloc(sizeof (fp_map_t));

  if (!is_const) {
    item->key = key;
  } else {
    item->key = malloc(strlen(key) + 1);
    strcpy(item->key, key);
  }

  item->fp = fp;

  HASH_ADD_STR(*uniq_fp_map, key, item);

  return fp;
}

// Splits a db data stream on stdin into multiple output files.
void
split(options_t *options) {
  // Read and parse the #db header of the input data.
  char *header = read_header(stdin);
  schema_t schema;
  parse_header(header, &schema);

  int *indexes = NULL;
  if (options->keylen) {
    // Figure out the indexes of the key columns.
    indexes = get_indexes(options->key, options->keylen, &schema);
  }

#ifdef DEBUG
  free_schema(&schema);
#endif

  int part = 0; // Partition counter.

  fp_map_t *uniq_fp_map = NULL; // Maps keys to file pointers in uniq mode.

  // Stores output file pointers when not using uniq mode.
  FILE **fps = NULL;
  if (!options->uniq) {
    fps = open_output_files(options, header);
  }

  // Allocate line buffers.
  size_t bufsize = BUFSIZE;
  char *line = malloc(bufsize);
  char *copy = malloc(bufsize);
  size_t offset = 0;

  char *key_tokens[options->keylen];
  size_t key_token_sizes[options->keylen];
  if (options->set || options->uniq || 1) {
    // In both set and uniq mode, we must operate on all of the key values at
    // once. Initialize an array to store the key values.
    int i;
    for (i=0; i<options->keylen; i++) {
      key_tokens[i] = malloc(bufsize);
      key_token_sizes[i] = bufsize;
    }
  }

  XXH32_stateSpace_t state;

  // Read lines from the input data.
  while (fgets(line + offset, bufsize - offset, stdin)) {
    if (line[strlen(line)-1] == '\n') {
      offset = 0;
    } else {
      // fgets() did not read an entire line. Grow the buffer and try again.
      bufsize *= 2;

#ifdef DEBUG
      fprintf(stderr, "line buffer doubled to %lu bytes\n", bufsize);
#endif

      line = realloc(line, bufsize);
      copy = realloc(copy, bufsize);

      // Set offset so that the next fgets() is concatenated to the buffer.
      offset = strlen(line);
      continue;
    }

    FILE *fp = NULL;

    ///
    // Determine the appropriate output file.
    ///

    if (options->keylen) {
      ///
      // Partition by key.
      ///

      // Copy the line so we can use strtok().
      strcpy(copy, line);

      XXH32_resetState(&state, 0);

      // Tokenize to find the key column values.
      char *token = strtok(copy, "\t\n");
      int token_index = 1;
      int j;
      for (j=0; j<options->keylen; j++) {
        int key_index = indexes[j];

        // Tokenize up to the key value.
        while (token_index < key_index) {
          token = strtok(NULL, "\t\n");
          token_index++;
        }

        if (options->set || options->uniq) {
          // Grow the token buffer if necessary.
          while (key_token_sizes[j] <= strlen(token)) {
            key_token_sizes[j] *= 2;
            key_tokens[j] = realloc(key_tokens[j], key_token_sizes[j]);

#ifdef DEBUG
            fprintf(stderr, "doubled token buffer %d to %lu bytes\n",
                    j, key_token_sizes[j]);
#endif
          }

          // Save the token for later.
          strcpy(key_tokens[j], token);
        } else {
          // Ordered, non-uniq mode. Just update the hash.
          XXH32_update(&state, token, strlen(token));
        }
      }

      if (options->set) {
        // Sort the key values.
        qsort(key_tokens, options->keylen, sizeof (char *), cmp_str);
      }

      if (options->uniq) {
        // Concatenate all of the key values into a key string.
        size_t total = 1;
        for (j=0; j<options->keylen; j++) {
          total += strlen(key_tokens[j]);
        }
        char *key = malloc(total);
        key[0] = '\0';
        for (j=0; j<options->keylen; j++) {
          strcat(key, key_tokens[j]);
        }

        // Map the key to a file pointer.
        fp = get_uniq_fp(&uniq_fp_map, key, options->prefix, &part, header, 0);
      } else {
        if (options->set) {
          // Update the hash with all key values.
          for (j=0; j<options->keylen; j++) {
            XXH32_update(&state, key_tokens[j], strlen(key_tokens[j]));
          }
        }

        // Choose the partition based on the hash value.
        fp = fps[XXH32_intermediateDigest(&state) % options->parts];
      }
    } else {
      ///
      // Partition by the entire line.
      ///

      if (!options->uniq) {
        // Round-robin split.
        fp = fps[part++];
        if (part > options->parts-1) {
          part = 0;
        }
      } else {
        // Uniq split; map the key to a file pointer.
        fp = get_uniq_fp(&uniq_fp_map, line, options->prefix, &part, header,
                         1);
      }
    }

    // Output the line to the output file.
    fwrite(line, 1, strlen(line), fp);
  }

  // Close output files.
  if (fps) {
    int i;
    for (i=0; i<options->parts; i++) {
      if (fclose(fps[i]) == EOF) {
        perror("fclose() error");
        exit(-errno);
      }
    }
  } else {
    fp_map_t *item;
    fp_map_t *tmp;
    HASH_ITER(hh, uniq_fp_map, item, tmp) {
      if (fclose(item->fp) == EOF) {
        perror("fclose() error");
        exit(-errno);
      }
    }
  }

#ifdef DEBUG
  free(header);

  if (indexes) {
    free(indexes);
  }

  if (fps) {
    free(fps);
  }

  int i;
  for (i=0; i<options->keylen; i++) {
    free(key_tokens[i]);
  }

  free(copy);
  free(line);

  fp_map_t *item;
  fp_map_t *tmp;
  HASH_ITER(hh, uniq_fp_map, item, tmp) {
    HASH_DEL(uniq_fp_map, item);
    free(item->key);
    free(item);
  }
#endif
}

int
main(int argc, char **argv) {
  static struct option lopts[] = {
    {"help", no_argument, NULL, 'h'},
    {"parts", required_argument, NULL, 'n'},
    {"key", required_argument, NULL, 'k'},
    {"set", no_argument, NULL, 's'},
    {"prefix", required_argument, NULL, 'p'},
    {"uniq", no_argument, NULL, 'u'},
    {NULL, 0, NULL, 0}
  };
  const char *sopts = "hn:k:sp:u";
  char opt;
  char *key = NULL;

  options_t options = {2, NULL, 0, 0, "split", NULL, 0};

  // Parse arguments.
  while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
    switch (opt) {
      case 'n':
        options.parts = strtol(optarg, NULL, 10);
        break;
      case 'k':
        key = optarg;
        break;
      case 's':
        options.set = 1;
        break;
      case 'p':
        options.prefix = optarg;
        break;
      case 'u':
        options.uniq = 1;
        break;
      default:
        printf("Usage: [data] | %s [OPTIONS] [OUTPUT FILE x n]\n\n", argv[0]);
        printf("-h | --help           print this text and exit\n");
        printf("-n | --parts          number of output partitions\n");
        printf("-k | --key            comma-separated list of key columns\n");
        printf("-s | --set            treat [key] as a set\n");
        printf("-p | --prefix         output file prefix\n");
        printf("-u | --uniq           put each key in its own partition\n\n");
        printf("Examples:\n\n");
        printf("10-way, round-robin split:\n");
        printf("[data] | %s -n 10\n\n", argv[0]);
        printf("10-way partition on columns 'sip' and 'dip':\n");
        printf("[data] | %s -k sip,dip -n 10\n\n", argv[0]);
        printf("10-way partition on the set {sip, dip}:\n");
        printf("[data] | %s -k sip,dip -s -n 10\n\n", argv[0]);
        printf("Each unique value of 'sip' in its own partition:\n");
        printf("[data] | %s -k sip -u\n", argv[0]);
        return 0;
    }
  }

  if (options.uniq) {
    options.parts = 0;
  } else if (options.parts < 2) {
    fprintf(stderr, "-n (--parts) must be at least 2\n");
    return 1;
  }

  uint32_t nargs = argc - optind;
  if (nargs) {
    ///
    // Output file names are specified on the command line.
    ///

    if (options.uniq) {
      // We don't know the number of partitions!
      fprintf(stderr, "output file names may not be used with -u (--uniq)\n");
      return 1;
    }

    // If output file names are specified, there must be [parts] names.
    if (nargs != options.parts) {
      fprintf(stderr, "expecting %d output file names, found %d\n",
              options.parts, nargs);
      return 1;
    }

    // Store the names in the options structure.
    options.outputs = malloc(sizeof (char *) * nargs);
    int i;
    for (i=0; i<nargs; i++) {
      options.outputs[i] = argv[optind+i];
    }
  }

  if (key) {
    ///
    // Tokenize the key column list.
    ///

    int cap = 2;
    options.key = malloc(sizeof (char *) * cap);

    const char *delim = ", ";
    char *token = strtok(key, delim);
    while (token) {
      if (options.keylen == cap) {
        cap *= 2;
        options.key = realloc(options.key, sizeof (char *) * cap);
      }

      options.key[options.keylen] = malloc(strlen(token) + 1);
      strcpy(options.key[options.keylen], token);
      options.keylen++;
      token = strtok(NULL, delim);
    }
  }

  split(&options);

#ifdef DEBUG
  if (options.keylen) {
    int i;
    for (i=0; i<options.keylen; i++) {
      free(options.key[i]);
    }
    free(options.key);
  }

  if (options.outputs) {
    free(options.outputs);
  }
#endif

  return 0;
}
