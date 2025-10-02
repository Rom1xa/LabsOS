#include <dirent.h>
#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_FILES 1000
#define MAX_PATH 1024

#define RESET "\033[0m"
#define BLUE "\033[34m"
#define GREEN "\033[32m"
#define CYAN "\033[36m"

typedef struct {
  char name[256];
  char full_path[MAX_PATH];
  struct stat stat_info;
} file_info_t;

static int opt_l = 0;
static int opt_a = 0;

void get_permissions(mode_t mode, char *perms) {
  perms[0] = (S_ISDIR(mode)) ? 'd' : (S_ISLNK(mode)) ? 'l' : '-';
  perms[1] = (mode & S_IRUSR) ? 'r' : '-';
  perms[2] = (mode & S_IWUSR) ? 'w' : '-';
  perms[3] = (mode & S_IXUSR) ? 'x' : '-';
  perms[4] = (mode & S_IRGRP) ? 'r' : '-';
  perms[5] = (mode & S_IWGRP) ? 'w' : '-';
  perms[6] = (mode & S_IXGRP) ? 'x' : '-';
  perms[7] = (mode & S_IROTH) ? 'r' : '-';
  perms[8] = (mode & S_IWOTH) ? 'w' : '-';
  perms[9] = (mode & S_IXOTH) ? 'x' : '-';
  perms[10] = '\0';
}

const char *get_file_color(mode_t mode) {
  if (S_ISDIR(mode))
    return BLUE;
  if (S_ISLNK(mode))
    return CYAN;
  if (mode & S_IXUSR || mode & S_IXGRP || mode & S_IXOTH)
    return GREEN;
  return RESET;
}

void format_time(time_t time_val, char *time_str) {
  struct tm *tm_info = localtime(&time_val);
  strftime(time_str, 20, "%b %d %H:%M", tm_info);
}

int compare_files(const void *a, const void *b) {
  const file_info_t *file_a = (const file_info_t *)a;
  const file_info_t *file_b = (const file_info_t *)b;
  return strcasecmp(file_a->name, file_b->name);
}

static const char *user_or_uid(uid_t uid, char *buf, size_t bufsz) {
  struct passwd *pwd = getpwuid(uid);
  if (pwd && pwd->pw_name)
    return pwd->pw_name;
  snprintf(buf, bufsz, "%lu", (unsigned long)uid);
  return buf;
}

static const char *group_or_gid(gid_t gid, char *buf, size_t bufsz) {
  struct group *grp = getgrgid(gid);
  if (grp && grp->gr_name)
    return grp->gr_name;
  snprintf(buf, bufsz, "%lu", (unsigned long)gid);
  return buf;
}

void print_long_format(file_info_t *files, int count) {
  nlink_t max_links = 0;
  off_t max_size = 0;
  char perms[11], time_str[20];

  for (int i = 0; i < count; i++) {
    if (files[i].stat_info.st_nlink > max_links)
      max_links = files[i].stat_info.st_nlink;
    if (files[i].stat_info.st_size > max_size)
      max_size = files[i].stat_info.st_size;
  }

  for (int i = 0; i < count; i++) {
    get_permissions(files[i].stat_info.st_mode, perms);
    format_time(files[i].stat_info.st_mtime, time_str);

    char ubuf[32], gbuf[32];
    const char *username =
        user_or_uid(files[i].stat_info.st_uid, ubuf, sizeof ubuf);
    const char *groupname =
        group_or_gid(files[i].stat_info.st_gid, gbuf, sizeof gbuf);

    printf("%s %*ld %-8s %-8s %*ld %s ", perms, (int)strlen("1234567890"),
           (long)files[i].stat_info.st_nlink, username, groupname,
           (int)strlen("1234567890"), (long)files[i].stat_info.st_size,
           time_str);

    const char *color = get_file_color(files[i].stat_info.st_mode);
    printf("%s%s%s", color, files[i].name, RESET);

    if (S_ISDIR(files[i].stat_info.st_mode)) {
      printf("%s/%s", color, RESET);
    } else if (files[i].stat_info.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
      printf("*");
    }

    printf("\n");
  }
}

void print_normal_format(file_info_t *files, int count) {
  for (int i = 0; i < count; i++) {
    const char *color = get_file_color(files[i].stat_info.st_mode);
    printf("%s%s%s", color, files[i].name, RESET);

    if (S_ISDIR(files[i].stat_info.st_mode)) {
      printf("%s/%s", color, RESET);
    } else if (files[i].stat_info.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
      printf("*");
    }

    printf("  ");
  }
  if (count > 0)
    printf("\n");
}

int process_directory(const char *dir_path) {
  DIR *dir;
  struct dirent *entry;
  file_info_t files[MAX_FILES];
  int file_count = 0;

  dir = opendir(dir_path);
  if (dir == NULL) {
    perror("opendir");
    return -1;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (!opt_a && entry->d_name[0] == '.')
      continue;
    if (file_count >= MAX_FILES) {
      fprintf(stderr, "Too many files in directory\n");
      break;
    }

    strcpy(files[file_count].name, entry->d_name);
    snprintf(files[file_count].full_path, MAX_PATH, "%s/%s", dir_path,
             entry->d_name);

    if (stat(files[file_count].full_path, &files[file_count].stat_info) == -1) {
      perror("stat");
      continue;
    }

    file_count++;
  }

  closedir(dir);

  if (file_count == 0)
    return 0;

  qsort(files, file_count, sizeof(file_info_t), compare_files);

  if (opt_l) {
    print_long_format(files, file_count);
  } else {
    print_normal_format(files, file_count);
  }

  return 0;
}

int parse_arguments(int argc, char *argv[], char **target_dir) {
  int opt;
  int option_index = 0;

  static struct option long_options[] = {{"help", no_argument, 0, 'h'},
                                         {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "la", long_options, &option_index)) !=
         -1) {
    switch (opt) {
    case 'l':
      opt_l = 1;
      break;
    case 'a':
      opt_a = 1;
      break;
    case 'h':
      printf("Usage: %s [OPTIONS] [DIRECTORY]\n", argv[0]);
      printf("Options:\n");
      printf("  -l    use a long listing format\n");
      printf("  -a    do not ignore entries starting with .\n");
      printf("  -h    display this help and exit\n");
      return 1;
    case '?':
      fprintf(stderr, "Unknown option: %c\n", optopt);
      return -1;
    default:
      return -1;
    }
  }

  if (optind < argc)
    *target_dir = argv[optind];
  else
    *target_dir = ".";

  return 0;
}

int main(int argc, char *argv[]) {
  char *target_dir;
  int result;

  result = parse_arguments(argc, argv, &target_dir);
  if (result != 0)
    return result == 1 ? 0 : 1;

  result = process_directory(target_dir);
  if (result != 0)
    return 1;

  return 0;
}
