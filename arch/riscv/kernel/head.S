    .section .text.init
    .globl _start, _ekernel
    .extern start_kernel

_start:
    # 0. 设置 sp 为 _ekernel 之后 4 KiB 的位置
    # li a0, 114514
    la t0, _ekernel
    li t1, 0x1000
    add sp, t0, t1

    # 1. 跳转到 start_kernel
    j start_kernel
