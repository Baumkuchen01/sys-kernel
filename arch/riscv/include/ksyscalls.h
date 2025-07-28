#ifndef __KSYSCALLS_H__
#define __KSYSCALLS_H__

#include <stddef.h>
#include <proc.h>

void syscall_handler(struct pt_regs *regs);
long sys_write(int fd, const char *buf, size_t count);
long sys_getpid(void);
struct pt_regs;
long sys_clone(struct pt_regs *regs);
long sys_brk(unsigned long brk);
long sys_read(int fd, char *buf, size_t count);
long sys_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
long sys_kill(pid_t pid, int sig);
void sys_rt_sigreturn(struct pt_regs *regs);

#endif
