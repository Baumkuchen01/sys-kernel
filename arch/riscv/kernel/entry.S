#include <private_kdefs.h>

    .section .text.entry
    .align 4
    .globl _traps
_traps:
    # 1. 将寄存器和 sepc 保存到栈上
    sd x1, -0*8(sp)
    sd x2, -1*8(sp)
    sd x3, -2*8(sp)
    sd x4, -3*8(sp)
    sd x5, -4*8(sp)
    sd x6, -5*8(sp)
    sd x7, -6*8(sp)
    sd x8, -7*8(sp)
    sd x9, -8*8(sp)
    sd x10, -9*8(sp)
    sd x11, -10*8(sp)
    sd x12, -11*8(sp)
    sd x13, -12*8(sp)
    sd x14, -13*8(sp)
    sd x15, -14*8(sp)
    sd x16, -15*8(sp)
    sd x17, -16*8(sp)
    sd x18, -17*8(sp)
    sd x19, -18*8(sp)
    sd x20, -19*8(sp)
    sd x21, -20*8(sp)
    sd x22, -21*8(sp)
    sd x23, -22*8(sp)
    sd x24, -23*8(sp)
    sd x25, -24*8(sp)
    sd x26, -25*8(sp)
    sd x27, -26*8(sp)
    sd x28, -27*8(sp)
    sd x29, -28*8(sp)
    sd x30, -29*8(sp)
    sd x31, -30*8(sp)
    csrr a0, scause
    csrr a1, sepc
    sd a1, -31*8(sp)

    addi sp, sp, -32*8
    
    # 2. 调用 trap_handler
    call trap_handler

    # 3. 恢复寄存器和 sepc
    #    特别注意 sp 寄存器的恢复
    addi sp, sp, 32*8
    ld x1, -0*8(sp)
    ld x2, -1*8(sp)
    ld x3, -2*8(sp)
    ld x4, -3*8(sp)
    ld x5, -4*8(sp)
    ld x6, -5*8(sp)
    ld x7, -6*8(sp)
    ld x8, -7*8(sp)
    ld x9, -8*8(sp)
    ld x10, -9*8(sp)
    ld x11, -10*8(sp)
    ld x12, -11*8(sp)
    ld x13, -12*8(sp)
    ld x14, -13*8(sp)
    ld x15, -14*8(sp)
    ld x16, -15*8(sp)
    ld x17, -16*8(sp)
    ld x18, -17*8(sp)
    ld x19, -18*8(sp)
    ld x20, -19*8(sp)
    ld x21, -20*8(sp)
    ld x22, -21*8(sp)
    ld x23, -22*8(sp)
    ld x24, -23*8(sp)
    ld x25, -24*8(sp)
    ld x26, -25*8(sp)
    ld x27, -26*8(sp)
    ld x28, -27*8(sp)
    ld x29, -28*8(sp)
    ld x30, -29*8(sp)
    ld x31, -30*8(sp)
    ld t0, -31*8(sp)
    csrw sepc, t0
    
    # 4. 返回
    sret

  .globl __dummy
__dummy:
    la a0, dummy_task
    csrw sepc, a0
    sret

  .globl __switch_to
__switch_to:
    # 1. 将当前线程的上下文保存到当前线程的 thread_struct 中
    sd ra, TS_OFF(a0)
    sd sp, (TS_OFF + 8)(a0)
    sd s0, (TS_OFF + 16)(a0)
    sd s1, (TS_OFF + 24)(a0)
    sd s2, (TS_OFF + 32)(a0)
    sd s3, (TS_OFF + 40)(a0)
    sd s4, (TS_OFF + 48)(a0)
    sd s5, (TS_OFF + 56)(a0)
    sd s6, (TS_OFF + 64)(a0)
    sd s7, (TS_OFF + 72)(a0)
    sd s8, (TS_OFF + 80)(a0)
    sd s9, (TS_OFF + 88)(a0)
    sd s10, (TS_OFF + 96)(a0)
    sd s11, (TS_OFF + 104)(a0)

    # 2. 从下一个线程的 thread_struct 中恢复上下文
    ld ra, TS_OFF(a1)
    ld sp, (TS_OFF + 8)(a1)
    ld s0, (TS_OFF + 16)(a1)
    ld s1, (TS_OFF + 24)(a1)
    ld s2, (TS_OFF + 32)(a1)
    ld s3, (TS_OFF + 40)(a1)
    ld s4, (TS_OFF + 48)(a1)
    ld s5, (TS_OFF + 56)(a1)
    ld s6, (TS_OFF + 64)(a1)
    ld s7, (TS_OFF + 72)(a1)
    ld s8, (TS_OFF + 80)(a1)
    ld s9, (TS_OFF + 88)(a1)
    ld s10, (TS_OFF + 96)(a1)
    ld s11, (TS_OFF + 104)(a1)

    ret