#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SV_IMPLEMENTATION
#include "./sv.h"

void usage(FILE *stream) { fprintf(stream, "Usage: ./minicel <input.csv>\n"); }

char *slurp_file(const char *file_path, size_t *size) {
  char *buffer = NULL;

  FILE *f = fopen(file_path, "rb");
  if (f == NULL) {
    goto error;
  }

  if (fseek(f, 0, SEEK_END) < 0) {
    goto error;
  }

  long m = ftell(f);
  if (m < 0) {
    goto error;
  }

  buffer = malloc(sizeof(char) * m);

  if (buffer == NULL) {
    goto error;
  }

  if (fseek(f, 0, SEEK_SET) < 0) {
    goto error;
  }
  // return how much we read
  size_t n = fread(buffer, 1, m, f);
  assert(n == (size_t)m);

  if (ferror(f)) {
    goto error;
  }
  if (size) {
    *size = n;
  }

  fclose(f);

  return buffer;

error:
  if (f) {
    fclose(f);
  }

  if (buffer) {
    free(buffer);
  }
  return NULL;
}

typedef enum {
  CELL_KIND_TEXT,
  CELL_KIND_NUMBER,
  CELL_KIND_EXPR,
} Cell_Kind;

typedef struct {
  Cell_Kind kind;
} Cell;

void estimate_table_size(String_View content, size_t *out_rows, size_t *out_cols) {
  size_t rows = 0;
  size_t cols = 0;

  for (; content.count > 0; ++rows ) {
    String_View line = sv_chop_by_delim(&content, '\n');
    size_t col = 0;
    for (; line.count > 0; ++col) {
      sv_chop_by_delim(&line, '|');
    }
    if (cols < col) {
      cols = col;
    }
  }
  if (out_cols) {
    *out_cols = cols;
  }
  if (out_rows) {
    *out_rows = rows;
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage(stderr);
    fprintf(stderr, "ERROR: input file has not been provided\n");
    exit(1);
  }

  const char *input_file_path = argv[1];

  size_t content_size = 0;

  char *content = slurp_file(input_file_path, &content_size);
  if (content == NULL) {
    fprintf(stderr, "ERROR: could not read file %s: %s\n", input_file_path,
            strerror(errno));
    exit(1);
  }

  String_View input = {.count = content_size, .data = content};


  size_t rows, cols;
  estimate_table_size(input, &rows, &cols);
  printf("Size of the table: %zux%zu\n", rows, cols);
  return 0;
}
