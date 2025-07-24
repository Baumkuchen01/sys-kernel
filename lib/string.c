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

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

size_t strlen(const char *str) {
    size_t len = 0;
    while (*str++)
        len++;
    return len;
}

char *strcpy(char *restrict dst, const char *restrict src) {
    char *d = dst;
    const char *s = src;

    while ((*d++ = *s++) != '\0');
    return dst;
}