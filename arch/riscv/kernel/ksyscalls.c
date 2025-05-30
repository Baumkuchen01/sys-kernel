#include <ksyscalls.h>
#include <proc.h>
#include <syscalls.h>
#include <printk.h>
#include <private_kdefs.h>
#include <vma.h>

extern struct task_struct *current;

void syscall_handler(struct pt_regs *regs) {
    unsigned long syscall_num = regs->x[17];
    switch (syscall_num) {
        case __NR_write:
            regs->x[10] = sys_write((unsigned)regs->x[10], (const char *)regs->x[11], regs->x[12]);
            break;
        case __NR_getpid:
            regs->x[10] = sys_getpid();
            break;
        case __NR_clone:
            regs->x[10] = sys_clone(regs);
            break;
        case __NR_brk:
            regs->x[10] = sys_brk((unsigned long)regs->x[10]);
            break;
        default:
            printk("Unknown syscall: %lu\n", syscall_num);
            break;
    }
}

long sys_write(unsigned fd, const char *buf, size_t count) {
    if (fd == 1) {
        for (size_t i = 0; i < count; i++) {
            printk("%c", buf[i]);
        }
        return count;
    }
    return -1;
}

long sys_getpid(void) {
    return current->pid;
}

long sys_clone(struct pt_regs *regs) {
    return do_fork(regs);
}

long sys_brk(unsigned long brk) {
    struct mm_struct *mm = current->mm;
    if (brk == 0) {
        return PGROUNDUP(mm->brk);
    }

    unsigned long oldbrk, newbrk, origbrk, minbrk;
    oldbrk = PGROUNDUP(mm->brk);
    newbrk = PGROUNDUP(brk);
    origbrk = mm->brk;
    minbrk = mm->start_brk;

    if (newbrk < minbrk) {
        return origbrk;
    }

    if (newbrk == oldbrk) {
        return brk;
    }

    if (newbrk < oldbrk) {
        struct vm_area_struct* brkvma = find_vma(mm, (void *)oldbrk);
        pagetable_t pgtbl = (pagetable_t)((((uint64_t)current->pgd & 0xfffffffffff) << 12) + PA2VA_OFFSET);
        do_vma_munmap(pgtbl, brkvma, newbrk, oldbrk);
        mm->brk = brk;
        return brk;
    } else {
        struct vm_area_struct* brkvma = find_vma(mm, (void *)oldbrk);
        do_vma_mmap(brkvma, oldbrk, newbrk);
        mm->brk = brk;
        return brk;
    }
}