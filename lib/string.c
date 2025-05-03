#include <string.h>
#include <stdint.h>

void *memset(void *restrict dst, int c, size_t n) {
    for (size_t i = 0; i < n; i++)
        ((uint8_t *)dst)[i] = c;
    return dst;
}

size_t strnlen(const char *restrict s, size_t maxlen) {
    size_t i;
    for(i = 0; i < maxlen && s[i]; i++);
    return i;
}

void *memcpy(void *restrict dst, const void *restrict src, size_t n)
{
  char *d = (char *)dst;
  const char *s = (const char *)src;
  
  for (size_t i = 0; i < n; i++) {
    d[i] = s[i];
  }
  
  return dst;
}
