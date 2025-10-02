#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *prog) {
  fprintf(stderr, "Usage: %s PATTERN [FILE...]\n", prog);
}

static int grep_stream(FILE *fp, const char *name, const char *pattern) {
  size_t cap = 0;
  char *line = NULL;
  size_t n;
  int ret = 0;

  while ((n = getline(&line, &cap, fp)) != -1) {
    if (strstr(line, pattern) != NULL) {
      fputs(line, stdout);
    }
  }

  if (ferror(fp)) {
    fprintf(stderr, "mygrep: read error on %s: %s\n", name ? name : "stdin",
            strerror(errno));
    ret = 1;
  }
  free(line);
  return ret;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage(argv[0]);
    return 2;
  }
  const char *pattern = argv[1];

  int status = 0;

  if (argc == 2) {
    status |= grep_stream(stdin, NULL, pattern);
  } else {
    for (int i = 2; i < argc; i++) {
      if (strcmp(argv[i], "-") == 0) {
        status |= grep_stream(stdin, NULL, pattern);
        clearerr(stdin);
      } else {
        FILE *fp = fopen(argv[i], "rb");
        if (!fp) {
          fprintf(stderr, "mygrep: cannot open %s: %s\n", argv[i],
                  strerror(errno));
          status = 1;
          continue;
        }
        status |= grep_stream(fp, argv[i], pattern);
        fclose(fp);
      }
    }
  }
  return status ? 1 : 0;
}
