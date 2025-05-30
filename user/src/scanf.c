#include <stdio.h>
#include <unistd.h>

static int scanf_syscall_read(FILE *restrict fp, void *restrict buf, size_t len) {
  (void)fp;
  return (int)read(STDIN_FILENO, buf, len);
}

int scanf(const char *restrict fmt, ...) {
  struct FILE scanf_in = {
      .read = scanf_syscall_read,
  };
  va_list ap;
  va_start(ap, fmt);
  int ret = vfscanf(&scanf_in, fmt, ap);
  va_end(ap);
  return ret;
}
