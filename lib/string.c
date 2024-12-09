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
