#ifndef __KSYSCALLS_H__
#define __KSYSCALLS_H__

#include <stddef.h>
#include <proc.h>

void syscall_handler(struct pt_regs *regs);
long sys_write(unsigned fd, const char *buf, size_t count);
long sys_getpid(void);
struct pt_regs;
long sys_clone(struct pt_regs *regs);
long sys_brk(unsigned long brk);

#endif
