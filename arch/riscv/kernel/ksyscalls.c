#include <ksyscalls.h>
#include <proc.h>
#include <syscalls.h>
#include <printk.h>
#include <private_kdefs.h>
#include <vma.h>
#include <sbi.h>
#include <string.h>
#include <signal.h>
#include <open.h>

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
        case __NR_kill:
            regs->x[10] = sys_kill((pid_t)regs->x[10], (int)regs->x[11]);
            break;
        case __NR_rt_sigreturn:
            sys_rt_sigreturn(regs);
            break;
        case __NR_openat:
            regs->x[10] = sys_openat((int)regs->x[10], (const char *)regs->x[11], (int)regs->x[12]);
            break;
        case __NR_close:
            regs->x[10] = sys_close((int)regs->x[10]);
            break;
        default:
            printk("Unknown syscall: %lu\n", syscall_num);
            break;
    }
}

long sys_write(unsigned fd, const char *buf, size_t len) {
    int64_t ret;
    struct file *file = &(current->files->fd_array[fd]);
    if (file->opened == 0) {
        printk("file not opened\n");
        return ERROR_FILE_NOT_OPEN;
    } else {
        // check perms and call write function of file
        if (file->perms & FILE_WRITABLE) {
            if (file->write) {
                ret = file->write(file, buf, len);
            } else {
                printk("File write function not implemented\n");
                // return ERROR_FILE_NOT_IMPLEMENTED;
            }
        } else {
            printk("File not writable\n");
            return ERROR_FILE_NOT_WRITABLE;
        }
    }
    return ret;
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

long sys_read(unsigned fd, char *buf, size_t count) {
    int64_t ret;
    struct file *file = &(current->files->fd_array[fd]);
    if (file->opened == 0) {
        printk("file not opened\n");
        return ERROR_FILE_NOT_OPEN;
    } else {
        // check perms and call read function of file
        if (file->perms & FILE_READABLE) {
            if (file->read) {
                ret = file->read(file, buf, count);
            } else {
                printk("File read function not implemented\n");
            }
        } else {
            printk("File not readable\n");
            return ERROR_FILE_NOT_READABLE;
        }
    }
    return ret;
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

void send_signal(struct task_struct *task, int sig) {
    struct signal_struct *signal = task->signal;

    signal->sigpending.sig[0] |= sigmask(sig);
    
    if (!(signal->blocked.sig[sig / __BITS_PER_LONG] & sigmask(sig))) {
        task->flags |= _TIF_SIGPENDING;
    }
}

long sys_kill(pid_t pid, int sig) {
    if (pid <= 0 || sig < 0 || sig >= _NSIG) {
        return -1;
    }

    struct task_struct *task = find_task_by_pid(pid);
    if (!task) {
        return -1;
    }

    send_signal(task, sig);
    return 0;
}

void sys_rt_sigreturn(struct pt_regs *regs) {
    struct rt_sigframe *frame = get_sigframe(regs);
    for (int i = 0; i < 32; i++) {
        regs->x[i] = frame->user_regs.x[i];
    }
    regs->sepc = frame->user_regs.sepc - 4;
    return;
}