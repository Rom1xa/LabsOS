#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUF_SIZE 65536

struct entry_header {
  uint32_t name_len;
  uint64_t content_len;
  uint32_t mode;
  uint32_t uid;
  uint32_t gid;
  int64_t mtime;
  uint8_t deleted;
};

static ssize_t read_full(int fd, void *buf, size_t count);
static int write_full(int fd, const void *buf, size_t count);

static int write_header(int fd, const struct entry_header *h) {
  if (write_full(fd, &h->name_len, sizeof(h->name_len)) < 0)
    return -1;
  if (write_full(fd, &h->content_len, sizeof(h->content_len)) < 0)
    return -1;
  if (write_full(fd, &h->mode, sizeof(h->mode)) < 0)
    return -1;
  if (write_full(fd, &h->uid, sizeof(h->uid)) < 0)
    return -1;
  if (write_full(fd, &h->gid, sizeof(h->gid)) < 0)
    return -1;
  if (write_full(fd, &h->mtime, sizeof(h->mtime)) < 0)
    return -1;
  if (write_full(fd, &h->deleted, sizeof(h->deleted)) < 0)
    return -1;
  return 0;
}

static int read_header(int fd, struct entry_header *h,
                       ssize_t *bytes_read_out) {
  ssize_t total = 0;
  ssize_t r;
  r = read_full(fd, &h->name_len, sizeof(h->name_len));
  if (r == 0) {
    if (bytes_read_out)
      *bytes_read_out = 0;
    return 0;
  }
  if (r < 0) {
    if (bytes_read_out)
      *bytes_read_out = r;
    return -1;
  }
  total += r;
  if ((size_t)r < sizeof(h->name_len)) {
    if (bytes_read_out)
      *bytes_read_out = total;
    errno = EINVAL;
    return -1;
  }
  r = read_full(fd, &h->content_len, sizeof(h->content_len));
  if (r <= 0) {
    if (bytes_read_out)
      *bytes_read_out = r;
    return -1;
  }
  total += r;
  if ((size_t)r < sizeof(h->content_len)) {
    if (bytes_read_out)
      *bytes_read_out = total;
    errno = EINVAL;
    return -1;
  }
  r = read_full(fd, &h->mode, sizeof(h->mode));
  if (r <= 0) {
    if (bytes_read_out)
      *bytes_read_out = r;
    return -1;
  }
  total += r;
  if ((size_t)r < sizeof(h->mode)) {
    if (bytes_read_out)
      *bytes_read_out = total;
    errno = EINVAL;
    return -1;
  }
  r = read_full(fd, &h->uid, sizeof(h->uid));
  if (r <= 0) {
    if (bytes_read_out)
      *bytes_read_out = r;
    return -1;
  }
  total += r;
  if ((size_t)r < sizeof(h->uid)) {
    if (bytes_read_out)
      *bytes_read_out = total;
    errno = EINVAL;
    return -1;
  }
  r = read_full(fd, &h->gid, sizeof(h->gid));
  if (r <= 0) {
    if (bytes_read_out)
      *bytes_read_out = r;
    return -1;
  }
  total += r;
  if ((size_t)r < sizeof(h->gid)) {
    if (bytes_read_out)
      *bytes_read_out = total;
    errno = EINVAL;
    return -1;
  }
  r = read_full(fd, &h->mtime, sizeof(h->mtime));
  if (r <= 0) {
    if (bytes_read_out)
      *bytes_read_out = r;
    return -1;
  }
  total += r;
  if ((size_t)r < sizeof(h->mtime)) {
    if (bytes_read_out)
      *bytes_read_out = total;
    errno = EINVAL;
    return -1;
  }
  r = read_full(fd, &h->deleted, sizeof(h->deleted));
  if (r <= 0) {
    if (bytes_read_out)
      *bytes_read_out = r;
    return -1;
  }
  total += r;
  if ((size_t)r < sizeof(h->deleted)) {
    if (bytes_read_out)
      *bytes_read_out = total;
    errno = EINVAL;
    return -1;
  }
  if (bytes_read_out)
    *bytes_read_out = total;
  return 1;
}

static ssize_t read_full(int fd, void *buf, size_t count) {
  size_t off = 0;
  while (off < count) {
    ssize_t r = read(fd, (char *)buf + off, count - off);
    if (r == 0)
      return off;
    if (r < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    off += (size_t)r;
  }
  return (ssize_t)off;
}

static int write_full(int fd, const void *buf, size_t count) {
  size_t off = 0;
  while (off < count) {
    ssize_t w = write(fd, (const char *)buf + off, count - off);
    if (w < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    off += (size_t)w;
  }
  return 0;
}

static int copy_bytes(int src_fd, int dst_fd, uint64_t bytes) {
  char buf[BUF_SIZE];
  uint64_t remaining = bytes;
  while (remaining > 0) {
    size_t chunk = remaining > BUF_SIZE ? BUF_SIZE : (size_t)remaining;
    ssize_t r = read_full(src_fd, buf, chunk);
    if (r <= 0)
      return -1;
    if (write_full(dst_fd, buf, (size_t)r) < 0)
      return -1;
    remaining -= (uint64_t)r;
  }
  return 0;
}

static int list_archive(const char *arch_path) {
  int fd = open(arch_path, O_RDONLY);
  if (fd < 0) {
    if (errno == ENOENT) {
      return 0;
    }
    perror("open");
    return 1;
  }
  struct entry_header hdr;
  while (1) {
    ssize_t r = 0;
    int rh = read_header(fd, &hdr, &r);
    if (rh == 0)
      break;
    if (rh < 0) {
      perror("read header");
      close(fd);
      return 1;
    }

    if (hdr.name_len == 0 || hdr.name_len > (1u << 20)) {
      fprintf(stderr, "corrupt archive (name_len)\n");
      close(fd);
      return 1;
    }
    char *name = (char *)malloc(hdr.name_len + 1);
    if (!name) {
      perror("malloc");
      close(fd);
      return 1;
    }
    if (read_full(fd, name, hdr.name_len) < 0) {
      perror("read name");
      free(name);
      close(fd);
      return 1;
    }
    name[hdr.name_len] = '\0';

    if (!hdr.deleted) {
      printf("%s\t%lu bytes\tmode %o\tuid %u\tgid %u\tmtime %ld\n", name,
             (unsigned long)hdr.content_len, hdr.mode, hdr.uid, hdr.gid,
             (long)hdr.mtime);
    }

    if (lseek(fd, (off_t)hdr.content_len, SEEK_CUR) == (off_t)-1) {
      if (copy_bytes(fd, -1, hdr.content_len) < 0) {
        free(name);
        close(fd);
        return 1;
      }
    }
    free(name);
  }
  close(fd);
  return 0;
}

static int rewrite_without(const char *arch_path, const char *remove_name,
                           const char *replace_name,
                           const char *replace_src_path) {
  char tmp_path[4096];
  snprintf(tmp_path, sizeof(tmp_path), "%s.tmp.%ld", arch_path, (long)getpid());

  int in_fd = open(arch_path, O_RDONLY | O_CREAT, 0644);
  if (in_fd < 0) {
    perror("open input archive");
    return 1;
  }
  int out_fd = open(tmp_path, O_WRONLY | O_TRUNC | O_CREAT, 0644);
  if (out_fd < 0) {
    perror("open temp");
    close(in_fd);
    return 1;
  }

  if (lseek(in_fd, 0, SEEK_SET) == (off_t)-1) {
    perror("lseek in");
    close(in_fd);
    close(out_fd);
    unlink(tmp_path);
    return 1;
  }

  struct entry_header hdr;
  while (1) {
    ssize_t r = 0;
    int rh = read_header(in_fd, &hdr, &r);
    if (rh == 0)
      break;
    if (rh < 0) {
      fprintf(stderr, "corrupt archive while copy\n");
      close(in_fd);
      close(out_fd);
      unlink(tmp_path);
      return 1;
    }

    if (hdr.name_len == 0 || hdr.name_len > (1u << 20)) {
      fprintf(stderr, "corrupt archive (name_len)\n");
      close(in_fd);
      close(out_fd);
      unlink(tmp_path);
      return 1;
    }
    char *name = (char *)malloc(hdr.name_len + 1);
    if (!name) {
      perror("malloc");
      close(in_fd);
      close(out_fd);
      unlink(tmp_path);
      return 1;
    }
    if (read_full(in_fd, name, hdr.name_len) < 0) {
      perror("read name");
      free(name);
      close(in_fd);
      close(out_fd);
      unlink(tmp_path);
      return 1;
    }
    name[hdr.name_len] = '\0';

    int should_remove =
        (remove_name && strcmp(name, remove_name) == 0 && !hdr.deleted);
    if (!should_remove) {
      if (write_header(out_fd, &hdr) < 0) {
        perror("write hdr");
        free(name);
        close(in_fd);
        close(out_fd);
        unlink(tmp_path);
        return 1;
      }
      if (write_full(out_fd, name, hdr.name_len) < 0) {
        perror("write name");
        free(name);
        close(in_fd);
        close(out_fd);
        unlink(tmp_path);
        return 1;
      }
      if (copy_bytes(in_fd, out_fd, hdr.content_len) < 0) {
        perror("copy content");
        free(name);
        close(in_fd);
        close(out_fd);
        unlink(tmp_path);
        return 1;
      }
    } else {
      if (lseek(in_fd, (off_t)hdr.content_len, SEEK_CUR) == (off_t)-1) {
        char buf[BUF_SIZE];
        uint64_t remaining = hdr.content_len;
        while (remaining > 0) {
          size_t chunk = remaining > BUF_SIZE ? BUF_SIZE : (size_t)remaining;
          ssize_t rr = read_full(in_fd, buf, chunk);
          if (rr <= 0) {
            perror("skip");
            free(name);
            close(in_fd);
            close(out_fd);
            unlink(tmp_path);
            return 1;
          }
          remaining -= (uint64_t)rr;
        }
      }
    }
    free(name);
  }

  // optionally append replacement (used by -i)
  if (replace_name && replace_src_path) {
    int src_fd = open(replace_src_path, O_RDONLY);
    if (src_fd < 0) {
      perror("open src");
      close(in_fd);
      close(out_fd);
      unlink(tmp_path);
      return 1;
    }
    struct stat st;
    if (fstat(src_fd, &st) < 0) {
      perror("fstat");
      close(src_fd);
      close(in_fd);
      close(out_fd);
      unlink(tmp_path);
      return 1;
    }
    struct entry_header nh;
    memset(&nh, 0, sizeof(nh));
    nh.name_len = (uint32_t)strlen(replace_name);
    nh.content_len = (uint64_t)st.st_size;
    nh.mode = (uint32_t)st.st_mode;
    nh.uid = (uint32_t)st.st_uid;
    nh.gid = (uint32_t)st.st_gid;
    nh.mtime = (int64_t)st.st_mtime;
    nh.deleted = 0;

    if (write_header(out_fd, &nh) < 0) {
      perror("write hdr");
      close(src_fd);
      close(in_fd);
      close(out_fd);
      unlink(tmp_path);
      return 1;
    }
    if (write_full(out_fd, replace_name, nh.name_len) < 0) {
      perror("write name");
      close(src_fd);
      close(in_fd);
      close(out_fd);
      unlink(tmp_path);
      return 1;
    }
    if (copy_bytes(src_fd, out_fd, nh.content_len) < 0) {
      perror("copy file");
      close(src_fd);
      close(in_fd);
      close(out_fd);
      unlink(tmp_path);
      return 1;
    }
    close(src_fd);
  }

  close(in_fd);
  if (fsync(out_fd) < 0) {
    perror("fsync");
  }
  close(out_fd);

  if (rename(tmp_path, arch_path) < 0) {
    perror("rename");
    unlink(tmp_path);
    return 1;
  }
  return 0;
}

static int add_file(const char *arch_path, const char *src_path) {
  const char *slash = strrchr(src_path, '/');
  const char *name = slash ? slash + 1 : src_path;
  return rewrite_without(arch_path, name, name, src_path);
}

static int extract_file(const char *arch_path, const char *want_name) {
  int fd = open(arch_path, O_RDONLY);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  struct entry_header hdr;
  int found = 0;
  while (1) {
    ssize_t r = 0;
    int rh = read_header(fd, &hdr, &r);
    if (rh == 0)
      break;
    if (rh < 0) {
      fprintf(stderr, "corrupt archive\n");
      close(fd);
      return 1;
    }
    if (hdr.name_len == 0 || hdr.name_len > (1u << 20)) {
      fprintf(stderr, "corrupt archive (name_len)\n");
      close(fd);
      return 1;
    }
    char *name = (char *)malloc(hdr.name_len + 1);
    if (!name) {
      perror("malloc");
      close(fd);
      return 1;
    }
    if (read_full(fd, name, hdr.name_len) < 0) {
      perror("read name");
      free(name);
      close(fd);
      return 1;
    }
    name[hdr.name_len] = '\0';

    if (!hdr.deleted && strcmp(name, want_name) == 0) {
      found = 1;
      int out_fd = open(name, O_WRONLY | O_TRUNC | O_CREAT, 0600);
      if (out_fd < 0) {
        perror("open out");
        free(name);
        close(fd);
        return 1;
      }
      if (copy_bytes(fd, out_fd, hdr.content_len) < 0) {
        perror("write out");
        free(name);
        close(out_fd);
        close(fd);
        return 1;
      }
      if (fchmod(out_fd, hdr.mode) < 0)
        perror("chmod");
      if (fchown(out_fd, hdr.uid, hdr.gid) < 0)
        perror("chown");
      struct timespec times[2];
      times[0].tv_sec = hdr.mtime;
      times[0].tv_nsec = 0;
      times[1].tv_sec = hdr.mtime;
      times[1].tv_nsec = 0;
      if (utimensat(AT_FDCWD, name, times, 0) < 0)
        perror("utimensat");
      close(out_fd);
      free(name);
      break;
    } else {
      if (lseek(fd, (off_t)hdr.content_len, SEEK_CUR) == (off_t)-1) {
        char buf[BUF_SIZE];
        uint64_t remaining = hdr.content_len;
        while (remaining > 0) {
          size_t chunk = remaining > BUF_SIZE ? BUF_SIZE : (size_t)remaining;
          ssize_t rr = read_full(fd, buf, chunk);
          if (rr <= 0) {
            perror("skip");
            free(name);
            close(fd);
            return 1;
          }
          remaining -= (uint64_t)rr;
        }
      }
    }
    free(name);
  }
  close(fd);
  if (!found) {
    fprintf(stderr, "file not found in archive: %s\n", want_name);
    return 1;
  }
  return rewrite_without(arch_path, want_name, NULL, NULL);
}

static void print_help(const char *prog) {
  dprintf(STDOUT_FILENO,
          "Usage:\n"
          "  %s ARCH -i|--input FILE\n"
          "  %s ARCH -e|--extract FILE\n"
          "  %s ARCH -s|--stat\n"
          "  %s -h|--help\n\n"
          "Notes:\n"
          "  - Archive format without compression.\n"
          "  - Uses open/read/write/lseek.\n"
          "  - Extract (-e) removes file from archive after writing.\n",
          prog, prog, prog, prog);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_help(argv[0]);
    return 1;
  }
  if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
    print_help(argv[0]);
    return 0;
  }
  if (argc < 3) {
    print_help(argv[0]);
    return 1;
  }

  const char *arch_path = argv[1];
  const char *opt = argv[2];

  if (!strcmp(opt, "-s") || !strcmp(opt, "--stat")) {
    return list_archive(arch_path);
  } else if ((!strcmp(opt, "-i") || !strcmp(opt, "--input")) && argc >= 4) {
    return add_file(arch_path, argv[3]);
  } else if ((!strcmp(opt, "-e") || !strcmp(opt, "--extract")) && argc >= 4) {
    return extract_file(arch_path, argv[3]);
  } else if (!strcmp(opt, "-h") || !strcmp(opt, "--help")) {
    print_help(argv[0]);
    return 0;
  } else {
    print_help(argv[0]);
    return 1;
  }
}
