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

extern FILE __iob[3];

int vfprintf(FILE *restrict f, const char *restrict fmt, va_list ap);
int printf(const char *restrict fmt, ...);

int vfscanf(FILE *restrict f, const char *restrict fmt, va_list ap);
int scanf(const char *restrict fmt, ...);

#endif
