#include <print.h>
#include <sbi.h>

void putc(char c) {
    sbi_ecall(0x4442434e, 2, c, 0, 0, 0, 0, 0);
}

void puts(const char *s) {
// #error Not yet implemented
    while (*s) {
        putc(*s);
        s++;
    }
}

void puti(int i) {
// #error Not yet implemented
    if (i < 0)
    {
        putc('-');
        i = -i;
    }
    if (i / 10 == 0)
        putc(i % 10 + '0');
    else 
    {
        puti(i / 10);
        putc(i % 10 + '0');
    }
}
