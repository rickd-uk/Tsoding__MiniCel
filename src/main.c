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

  for (size_t col = 0; input.count > 0; ++col) {
    String_View line = sv_chop_by_delim(&input, '\n');

    const char *start = line.data;
    for (size_t row = 0; line.count > 0; ++row) {
      String_View cell = sv_trim(sv_chop_by_delim(&line, '|'));
      printf("%s:%zu:%zu: (%zu, %zu) "SV_Fmt"\n", 
          input_file_path, 
          col, 
          cell.data - start,
          row, col,
          SV_Arg(cell));
    }
  }

  return 0;
}
