#include <ksyscalls.h>
#include <proc.h>
#include <syscalls.h>
#include <printk.h>

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
