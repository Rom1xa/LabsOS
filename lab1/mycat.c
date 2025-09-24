#include <errno.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  int number_all;      // -n
  int number_nonblank; // -b
  int show_ends;       // -E
} Options;

static void usage(const char *prog) {
  fprintf(stderr, "Usage: %s [-n] [-b] [-E] [FILE...]\n", prog);
}

static int cat_stream(FILE *fp, const char *name, const Options *opt) {
  int c;
  long lineno = 1;
  int at_line_start = 1;
  int current_line_nonblank = 0;

  while ((c = fgetc(fp)) != EOF) {
    if (at_line_start) {
      current_line_nonblank = 0;

      int d = c;
      if (d != '\n')
        current_line_nonblank = 1;

      int need_number = 0;
      if (opt->number_nonblank) {
        if (current_line_nonblank)
          need_number = 1;
      } else if (opt->number_all) {
        need_number = 1;
      }

      if (need_number) {
        printf("%6ld\t", lineno++);
      }
      at_line_start = 0;
    }

    if (c == '\n') {
      if (opt->show_ends)
        putchar('$');
      putchar('\n');
      at_line_start = 1;
    } else {
      putchar(c);
    }
  }

  if (ferror(fp)) {
    fprintf(stderr, "mycat: read error on %s: %s\n", name ? name : "stdin",
            strerror(errno));
    return 1;
  }
  return 0;
}

int main(int argc, char **argv) {
  Options opt = {0, 0, 0};
  int i = 1;

  while (i < argc && argv[i][0] == '-' && argv[i][1] != '\0') {
    if (strcmp(argv[i], "--") == 0) {
      i++;
      break;
    }

    for (int j = 1; argv[i][j] != '\0'; j++) {
      if (argv[i][j] == 'n')
        opt.number_all = 1;
      else if (argv[i][j] == 'b')
        opt.number_nonblank = 1;
      else if (argv[i][j] == 'E')
        opt.show_ends = 1;
      else {
        usage(argv[0]);
        return 2;
      }
    }
    i++;
  }

  if (opt.number_nonblank)
    opt.number_all = 0;

  int status = 0;

  if (i == argc) {
    // stdin
    status |= cat_stream(stdin, NULL, &opt);
  } else {
    for (; i < argc; i++) {
      if (strcmp(argv[i], "-") == 0) {
        status |= cat_stream(stdin, NULL, &opt);
        clearerr(stdin);
      } else {
        FILE *fp = fopen(argv[i], "rb");
        if (!fp) {
          fprintf(stderr, "mycat: cannot open %s: %s\n", argv[i],
                  strerror(errno));
          status = 1;
          continue;
        }
        status |= cat_stream(fp, argv[i], &opt);
        fclose(fp);
      }
    }
  }

  return status ? 1 : 0;
}
