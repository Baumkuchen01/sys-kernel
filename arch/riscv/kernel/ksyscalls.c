#include <ksyscalls.h>
#include <proc.h>
#include <syscalls.h>
#include <printk.h>
#include <private_kdefs.h>
#include <vma.h>
#include <sbi.h>
#include <string.h>
#include <signal.h>

#define MAX_READ_SIZE 256

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
        case __NR_read:
            regs->x[10] = sys_read((unsigned)regs->x[10], (char *)regs->x[11], regs->x[12]);
            break;
        case __NR_rt_sigaction:
            struct sigaction *act = (struct sigaction *)regs->x[11];
            struct sigaction *oldact = (struct sigaction *)regs->x[12];
            regs->x[10] = sys_sigaction((int)regs->x[10], act, oldact);
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
        do_vma_mmap(brkvma, newbrk);
        mm->brk = brk;
        return brk;
    }
}

static int sbi_read(const void *restrict buf, size_t len) {
    uint64_t base_addr_lo = VA2PA(buf);
    uint64_t base_addr_hi = 0;
    struct sbiret result = sbi_ecall(0x4442434e, 1, len, base_addr_lo, base_addr_hi, 0, 0, 0);
    return result.value;
}

long sys_read(unsigned fd, char *buf, size_t count) {
    if (fd != 0) return -1;
    if (count == 0) return 0;

    char kbuf[MAX_READ_SIZE];
    size_t bytes_read = 0;

    while (bytes_read < count) {
        size_t chunk = (count - bytes_read > MAX_READ_SIZE) 
                     ? MAX_READ_SIZE : count - bytes_read;
        if (chunk == 0) 
            break;
        long ret = sbi_read((void *)kbuf + bytes_read, chunk);
        bytes_read += ret;
    }

    memcpy(buf, kbuf, bytes_read);
    return bytes_read;
}

long sys_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
    if (signum < 1 || signum >= _NSIG || (act && (signum == SIGKILL || signum == SIGSTOP))) {
        return -1;
    }
    struct sighand_struct *sig = current->sighand;
    struct sigaction *k = &sig->action[signum - 1];

    if (oldact) {
        *oldact = *k;
    }

    if (act) {
        struct sigaction tmp, *newact = &tmp;
        *newact = *act;
        sigdelsetmask(&newact->sa_mask, sigmask(SIGKILL) | sigmask(SIGSTOP));
        *k = *newact;
    }

    return 0;
}