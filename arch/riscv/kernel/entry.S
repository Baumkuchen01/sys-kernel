#include <private_kdefs.h>

    .section .text.entry
    .align 4
    .globl _traps
_traps:
    # csrr t0, sscratch
    # beqz t0, L1
    # csrw sscratch, sp
    # mv sp, t0
# L1:
    csrrw sp, sscratch, sp
    # 1. 将寄存器和 sepc 保存到栈上
    addi sp, sp, -33 * 8
    sd x0, 0 * 8(sp)
    sd x1, 1 * 8(sp)
    sd x2, 2 * 8(sp)
    sd x3, 3 * 8(sp)
    sd x4, 4 * 8(sp)
    sd x5, 5 * 8(sp)
    sd x6, 6 * 8(sp)
    sd x7, 7 * 8(sp)
    sd x8, 8 * 8(sp)
    sd x9, 9 * 8(sp)
    sd x10, 10 * 8(sp)
    sd x11, 11 * 8(sp)
    sd x12, 12 * 8(sp)
    sd x13, 13 * 8(sp)
    sd x14, 14 * 8(sp)
    sd x15, 15 * 8(sp)
    sd x16, 16 * 8(sp)
    sd x17, 17 * 8(sp)
    sd x18, 18 * 8(sp)
    sd x19, 19 * 8(sp)
    sd x20, 20 * 8(sp)
    sd x21, 21 * 8(sp)
    sd x22, 22 * 8(sp)
    sd x23, 23 * 8(sp)
    sd x24, 24 * 8(sp)
    sd x25, 25 * 8(sp)
    sd x26, 26 * 8(sp)
    sd x27, 27 * 8(sp)
    sd x28, 28 * 8(sp)
    sd x29, 29 * 8(sp)
    sd x30, 30 * 8(sp)
    sd x31, 31 * 8(sp)
    csrr a0, sepc
    sd a0, 32 * 8(sp)

    mv a0, sp
    csrr a1, scause
    csrr a2, stval
    
    # 2. 调用 trap_handler
    call trap_handler

restore:
    # 3. 恢复寄存器和 sepc
    #    特别注意 sp 寄存器的恢复
    ld x1, 1 * 8(sp)
    ld x2, 2 * 8(sp)
    ld x3, 3 * 8(sp)
    ld x4, 4 * 8(sp)
    ld x5, 5 * 8(sp)
    ld x6, 6 * 8(sp)
    ld x7, 7 * 8(sp)
    ld x8, 8 * 8(sp)
    ld x9, 9 * 8(sp)
    ld x10, 10 * 8(sp)
    ld x11, 11 * 8(sp)
    ld x12, 12 * 8(sp)
    ld x13, 13 * 8(sp)
    ld x14, 14 * 8(sp)
    ld x15, 15 * 8(sp)
    ld x16, 16 * 8(sp)
    ld x17, 17 * 8(sp)
    ld x18, 18 * 8(sp)
    ld x19, 19 * 8(sp)
    ld x20, 20 * 8(sp)
    ld x21, 21 * 8(sp)
    ld x22, 22 * 8(sp)
    ld x23, 23 * 8(sp)
    ld x24, 24 * 8(sp)
    ld x25, 25 * 8(sp)
    ld x26, 26 * 8(sp)
    ld x27, 27 * 8(sp)
    ld x28, 28 * 8(sp)
    ld x29, 29 * 8(sp)
    ld x30, 30 * 8(sp)
    ld x31, 31 * 8(sp)
    ld t0, 32 * 8(sp)
    csrw sepc, t0
    addi sp, sp, 33 * 8

    # csrr t0, sscratch
    # beqz t0, L2
    # csrw sscratch, sp
    # mv sp, t0
# L2:
    csrrw sp, sscratch, sp
    # 4. 返回
    sret

    .globl __dummy
  __dummy:
    # la a0, dummy_task
    # csrw sepc, a0
    csrrw sp, sscratch, sp
    sret

    .globl __switch_to
  __switch_to:
    # 1. 将当前线程的上下文保存到当前线程的 thread_struct 中
    sd ra, (TS_OFF + 0 * 8)(a0)
    sd sp, (TS_OFF + 1 * 8)(a0)
    sd s0, (TS_OFF + 2 * 8)(a0)
    sd s1, (TS_OFF + 3 * 8)(a0)
    sd s2, (TS_OFF + 4 * 8)(a0)
    sd s3, (TS_OFF + 5 * 8)(a0)
    sd s4, (TS_OFF + 6 * 8)(a0)
    sd s5, (TS_OFF + 7 * 8)(a0)
    sd s6, (TS_OFF + 8 * 8)(a0)
    sd s7, (TS_OFF + 9 * 8)(a0)
    sd s8, (TS_OFF + 10 * 8)(a0)
    sd s9, (TS_OFF + 11 * 8)(a0)
    sd s10, (TS_OFF + 12 * 8)(a0)
    sd s11, (TS_OFF + 13 * 8)(a0)
    csrr t0, sepc
    sd t0, (TS_OFF + 14 * 8)(a0)
    csrr t0, sstatus
    sd t0, (TS_OFF + 15 * 8)(a0)
    csrr t0, sscratch
    sd t0, (TS_OFF + 16 * 8)(a0)
    csrr t0, stval
    sd t0, (TS_OFF + 17 * 8)(a0)
    csrr t0, scause
    sd t0, (TS_OFF + 18 * 8)(a0)
    csrr t0, satp
    sd t0, (TS_OFF + 19 * 8)(a0)


    # 2. 从下一个线程的 thread_struct 中恢复上下文
    ld ra, (TS_OFF + 0 * 8)(a1)
    ld sp, (TS_OFF + 1 * 8)(a1)
    ld s0, (TS_OFF + 2 * 8)(a1)
    ld s1, (TS_OFF + 3 * 8)(a1)
    ld s2, (TS_OFF + 4 * 8)(a1)
    ld s3, (TS_OFF + 5 * 8)(a1)
    ld s4, (TS_OFF + 6 * 8)(a1)
    ld s5, (TS_OFF + 7 * 8)(a1)
    ld s6, (TS_OFF + 8 * 8)(a1)
    ld s7, (TS_OFF + 9 * 8)(a1)
    ld s8, (TS_OFF + 10 * 8)(a1)
    ld s9, (TS_OFF + 11 * 8)(a1)
    ld s10, (TS_OFF + 12 * 8)(a1)
    ld s11, (TS_OFF + 13 * 8)(a1)
    ld t0, (TS_OFF + 14 * 8)(a1)
    csrw sepc, t0
    ld t0, (TS_OFF + 15 * 8)(a1)
    csrw sstatus, t0
    ld t0, (TS_OFF + 16 * 8)(a1)
    csrw sscratch, t0
    ld t0, (TS_OFF + 17 * 8)(a1)
    csrw stval, t0
    ld t0, (TS_OFF + 18 * 8)(a1)
    csrw scause, t0
    ld t0, (TS_OFF + 19 * 8)(a1)
    csrw satp, t0

    sfence.vma

    ret

    .globl ret_from_fork
ret_from_fork:
    j restore