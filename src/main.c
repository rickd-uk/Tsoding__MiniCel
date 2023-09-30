#include <stdio.h>
#include <stdlib.h>

void usage(FILE *stream) {
  fprintf(stream, "Usage: ./minicel <input.csv>\n");
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage(stderr);
    fprintf(stderr, "ERROR: input file has not been provided\n");
    exit(1);
  }

  const char *input_file_path = argv[1];



  return 0;
}
