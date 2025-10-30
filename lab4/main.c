#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void print_usage(const char *program_name) {
  printf("Использование: %s РЕЖИМ ФАЙЛ...\n", program_name);
}

int parse_numeric_mode(const char *mode_str) {
  char *endptr;
  long mode = strtol(mode_str, &endptr, 8);

  if (*endptr != '\0' || mode < 0 || mode > 0777) {
    return -1;
  }

  return (int)mode;
}

int parse_symbolic_mode(const char *mode_str, mode_t current_mode) {
  mode_t new_mode = current_mode;
  const char *p = mode_str;

  mode_t who = 0;
  mode_t what = 0;
  char op = 0;

  if (*p == '+' || *p == '-' || *p == '=') {
    who = S_IRWXU | S_IRWXG | S_IRWXO;
  } else {
    while (*p && *p != '+' && *p != '-' && *p != '=') {
      switch (*p) {
      case 'u':
        who |= S_IRWXU;
        break;
      case 'g':
        who |= S_IRWXG;
        break;
      case 'o':
        who |= S_IRWXO;
        break;
      case 'a':
        who |= S_IRWXU | S_IRWXG | S_IRWXO;
        break;
      default:
        return -1;
      }
      p++;
    }
  }

  if (*p == '\0')
    return -1;

  op = *p++;

  while (*p && *p != '\0') {
    switch (*p) {
    case 'r':
      if (who & S_IRWXU)
        what |= S_IRUSR;
      if (who & S_IRWXG)
        what |= S_IRGRP;
      if (who & S_IRWXO)
        what |= S_IROTH;
      break;
    case 'w':
      if (who & S_IRWXU)
        what |= S_IWUSR;
      if (who & S_IRWXG)
        what |= S_IWGRP;
      if (who & S_IRWXO)
        what |= S_IWOTH;
      break;
    case 'x':
      if (who & S_IRWXU)
        what |= S_IXUSR;
      if (who & S_IRWXG)
        what |= S_IXGRP;
      if (who & S_IRWXO)
        what |= S_IXOTH;
      break;
    default:
      return -1;
    }
    p++;
  }

  switch (op) {
  case '+':
    new_mode |= what;
    break;
  case '-':
    new_mode &= ~what;
    break;
  case '=':
    if (who & S_IRWXU)
      new_mode &= ~S_IRWXU;
    if (who & S_IRWXG)
      new_mode &= ~S_IRWXG;
    if (who & S_IRWXO)
      new_mode &= ~S_IRWXO;
    new_mode |= what;
    break;
  default:
    return -1;
  }

  return new_mode;
}

mode_t get_file_mode(const char *filename) {
  struct stat st;
  if (stat(filename, &st) == -1) {
    return (mode_t)-1;
  }
  return st.st_mode & 0777;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    print_usage(argv[0]);
    return 1;
  }

  const char *mode_str = argv[1];
  int new_mode = -1;

  if (mode_str[0] >= '0' && mode_str[0] <= '7') {
    new_mode = parse_numeric_mode(mode_str);
    if (new_mode == -1) {
      fprintf(stderr, "mychmod: неверный числовой режим '%s'\n", mode_str);
      return 1;
    }
  } else {
    mode_t current_mode = get_file_mode(argv[2]);
    if (current_mode == (mode_t)-1) {
      fprintf(stderr, "mychmod: не удалось получить права файла '%s': %s\n",
              argv[2], strerror(errno));
      return 1;
    }

    new_mode = parse_symbolic_mode(mode_str, current_mode);
    if (new_mode == -1) {
      fprintf(stderr, "mychmod: неверный символьный режим '%s'\n", mode_str);
      return 1;
    }

    for (int i = 2; i < argc; i++) {
      if (chmod(argv[i], new_mode) == -1) {
        fprintf(stderr, "mychmod: не удалось изменить права файла '%s': %s\n",
                argv[i], strerror(errno));
        continue;
      }
    }
    return 0;
  }

  for (int i = 2; i < argc; i++) {
    if (chmod(argv[i], new_mode) == -1) {
      fprintf(stderr, "mychmod: не удалось изменить права файла '%s': %s\n",
              argv[i], strerror(errno));
      return 1;
    }
  }

  return 0;
}

/* u+x */
