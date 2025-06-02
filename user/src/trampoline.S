#include <syscalls.h>

    .section .text
    .globl __trampoline

__trampoline:
    li a7, __NR_rt_sigreturn
    ecall