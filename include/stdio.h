#ifndef __STDIO_H__
#define __STDIO_H__

#include <stddef.h>
#include <stdarg.h>

#define UNGET_BUFSIZE 16
#define EOF (-1)

typedef struct FILE {
  char *str;
  size_t size;
  size_t offset;
  int (*write)(struct FILE *, const void *, size_t);
  unsigned char unget_buf[UNGET_BUFSIZE];
  int unget_count;
  int (*read)(struct FILE *, void *, size_t);
} FILE;

#define stdin (&__iob[0])
#define stdout (&__iob[1])
#define stderr (&__iob[2])

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define PURPLE "\033[35m"
#define DEEPGREEN "\033[36m"
#define CLEAR "\033[0m"

extern FILE __iob[3];

int vfprintf(FILE *restrict f, const char *restrict fmt, va_list ap);
int printf(const char *restrict fmt, ...);

int vfscanf(FILE *restrict f, const char *restrict fmt, va_list ap);
int scanf(const char *restrict fmt, ...);

#endif
