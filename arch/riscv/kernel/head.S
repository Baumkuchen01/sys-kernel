#include "private_kdefs.h"

    .section .text.init
    .globl _start

_start:
    # 0. 设置 sp 为 _ekernel 之后 4 KiB 的位置

    la sp, _sbss
    # li t0, 0xffffffdf80000000
    # sub sp, sp, t0

    call setup_vm
    call relocate

    call mm_init
    call setup_vm_final
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
    # li t0, 1 << 1
    # csrs sstatus, t0

    # 5. 跳转到 start_kernel
    j start_kernel
    
relocate:
    # 1. set general purpose registers to appropriate values
    #    - set ra = ra + PA2VA_OFFSET
    #    - set sp = sp + PA2VA_OFFSET, if needed

    li t0, 0xffffffdf80000000
    add ra, ra, t0
    add sp, sp, t0

    # flush TLB
    sfence.vma zero, zero

    # 2. set satp to use early_pgtbl
    #    - set satp to use Sv39 mode

    la t1, early_pgtbl
    # sub t1, t1, t0
    srli t1, t1, 12
    li t2, 8
    slli t2, t2, 60
    or t1, t1, t2
    csrw satp, t1

    ret 
    
    .section .bss.stack
    .space PGSIZE