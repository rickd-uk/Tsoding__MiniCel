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
  EXPR_KIND_NUM = 0,
  EXPR_KIND_CELL = 0,
  EXPR_KIND_BINARY_OP,
} Expr_Kind;

typedef struct {
  Expr_Kind kind;
} Expr;

typedef enum {
  CELL_KIND_TEXT = 0,
  CELL_KIND_NUMBER,
  CELL_KIND_EXPR,
} Cell_Kind;

const char *cell_kind_as_cstr(Cell_Kind kind) {
  switch (kind) {
  case CELL_KIND_TEXT:
    return "TEXT";
  case CELL_KIND_NUMBER:
    return "NUMBER";
  case CELL_KIND_EXPR:
    return "EXPR";
  default:
    assert(0 && "unreachable");
    exit(1);
  }
}

typedef union {
  String_View text;
  double number;
  Expr expr;
} Cell_As;

typedef struct {
  Cell_Kind kind;
  Cell_As as;
} Cell;

typedef struct {
  Cell *cells;
  size_t rows;
  size_t cols;
} Table;

Table table_alloc(size_t rows, size_t cols) {

  Table table = {0};
  table.rows = rows;
  table.cols = cols;
  table.cells = malloc(sizeof(Cell) * rows * cols);

  if (table.cells == NULL) {
    fprintf(stderr, "Error: could not allocate memory for the table\n");
    exit(1);
  }
  // zero initialize the table
  memset(table.cells, 0, sizeof(Cell) * rows * cols);

  return table;
}

Cell *table_cell_at(Table *table, size_t row, size_t col) {
  // simple boundary check
  assert(row < table->rows);
  assert(col < table->cols);
  return &table->cells[row * table->cols + col];
}

void parse_table_from_content(Table *table, String_View content) {
  for (size_t row = 0; content.count > 0; ++row) {
    String_View line = sv_chop_by_delim(&content, '\n');
    for (size_t col = 0; line.count > 0; ++col) {

      String_View cell_val = sv_trim(sv_chop_by_delim(&line, '|'));
      Cell *cell = table_cell_at(table, row, col);

      if (sv_starts_with(cell_val, SV("="))) {
        cell->kind = CELL_KIND_EXPR;
      } else {

        static char tmp_buf[1024 * 4];
        assert(cell_val.count < sizeof(tmp_buf));
        snprintf(tmp_buf, sizeof(tmp_buf), SV_Fmt, SV_Arg(cell_val));

        char *endptr;
        cell->as.number = strtod(tmp_buf, &endptr);

        if (endptr != tmp_buf && *endptr == '\0') {
          cell->kind = CELL_KIND_NUMBER;
        } else {
          cell->kind = CELL_KIND_TEXT;
          cell->as.text = cell_val;
        }
      }
    }
  }
}

void estimate_table_size(String_View content, size_t *out_rows,
                         size_t *out_cols) {
  size_t rows = 0;
  size_t cols = 0;

  for (; content.count > 0; ++rows) {
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
  Table table = table_alloc(rows, cols);
  parse_table_from_content(&table, input);

  for (size_t row = 0; row < table.rows; ++row) {
    for (size_t col = 0; col < table.cols; ++col) {
      Cell *cell = table_cell_at(&table, row, col);
      switch (cell->kind) {

      case CELL_KIND_TEXT:
        printf("TEXT(\""SV_Fmt"\")", SV_Arg(cell->as.text));
        break;
      case CELL_KIND_NUMBER:
        printf("NUMBER(%lf)", cell->as.number);
        break;
      case CELL_KIND_EXPR:
        printf("EXPR");
        break;
      }
      printf("|");
      /* printf("%s|", cell_kind_as_cstr(table_cell_at(&table, row, col)->kind)); */
    }
    printf("\n");
  }

  return 0;
}
