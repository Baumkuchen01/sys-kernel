#include "private_kdefs.h"

    .section .text.init
    .globl _start
    # .extern start_kernel

_start:
    # 0. 设置 sp 为 _ekernel 之后 4 KiB 的位置
    la sp, _sbss
    call mm_init
    call task_init

    # 1. 将 stvec 设置为 _traps
    la t0, _traps
    csrw stvec, t0

    # 2. 设置 sie[STIE]
    li t0, 1 << 5
    csrs sie, t0

    # 3. 设置第一次时钟中断的时间
    call clock_set_next_event

    # 4. 设置 sstatus[SIE]
    li t0, 1 << 1
    csrs sstatus, t0

    # 5. 跳转到 start_kernel
    j start_kernel

    .section .bss.stack
    .space PGSIZE