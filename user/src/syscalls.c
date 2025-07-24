#include <syscalls.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <fcntl.h>

void __trampoline(void);

pid_t getpid(void) {
  pid_t ret;
  asm volatile("li a7, %1\n\t"
               "ecall\n\t"
               "mv %0, a0\n\t"
               : "=r"(ret)
               : "i"(__NR_getpid)
               : "a0", "a7", "memory");
  return ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
  ssize_t ret;
  asm volatile("li a7, %1\n\t"
               "mv a0, %2\n\t"
               "mv a1, %3\n\t"
               "mv a2, %4\n\t"
               "ecall\n\t"
               "mv %0, a0\n\t"
               : "=r"(ret)
               : "i"(__NR_write), "r"(fd), "r"(buf), "r"(count)
               : "a0", "a1", "a2", "a7", "memory");
  return ret;
}

pid_t fork(void) {
  pid_t ret;
  asm volatile("li a7, %1\n\t"
         "ecall\n\t"
         "mv %0, a0\n\t"
         : "=r"(ret)
         : "i"(__NR_clone)
         : "a0", "a7", "memory");
  return ret;
}

void *brk(void *addr) {
  void *ret;
  asm volatile("li a7, %1\n\t"
               "mv a0, %2\n\t"
               "ecall\n\t"
               "mv %0, a0\n\t"
               : "=r"(ret)
               : "i"(__NR_brk), "r"(addr)
               : "a0", "a7", "memory");
  return ret;
}

void *sbrk(long long increment) {
  void *ret;
  static void *current_brk = NULL;
  void *new_brk;

  if (current_brk == NULL) {
    current_brk = brk(NULL);
  }

  ret = current_brk;
  new_brk = (void *)((char *)current_brk + increment);
  current_brk = brk(new_brk);

  if (current_brk != new_brk) {
    return (void *)-1;
  }

  return ret;
}

ssize_t read(int fd, void *buf, size_t count) {
  ssize_t ret;
  asm volatile("li a7, %1\n\t"
               "mv a0, %2\n\t"
               "mv a1, %3\n\t"
               "mv a2, %4\n\t"
               "ecall\n\t"
               "mv %0, a0\n\t"
               : "=r"(ret)
               : "i"(__NR_read), "r"(fd), "r"(buf), "r"(count)
               : "a0", "a1", "a2", "a7", "memory");
  return ret;
}

int sigaction(int signum, const struct sigaction *restrict act, struct sigaction *restrict oldact) {
  int ret;
  asm volatile("li a7, %1\n\t"
               "mv a0, %2\n\t"
               "mv a1, %3\n\t"
               "mv a2, %4\n\t"
               "ecall\n\t"
               "mv %0, a0\n\t"
               : "=r"(ret)
               : "i"(__NR_rt_sigaction), "r"(signum), "r"(act), "r"(oldact)
               : "a0", "a1", "a2", "a7", "memory");
  return ret;
}

sighandler_t signal(int signum, sighandler_t handler) {
  struct sigaction act, oldact;
  sighandler_t ret;
  
  act.sa_handler = handler;
  act.sa_flags = 0;
  for (int i = 0; i < _NSIG_WORDS; i++) {
    act.sa_mask.sig[i] = 0;
  }
  act.sa_restorer = __trampoline;
  
  if (sigaction(signum, &act, &oldact) < 0) {
    return SIG_ERR;
  }
  
  ret = oldact.sa_handler;
  return ret;
}

int kill(pid_t pid, int sig) {
  int ret;
  asm volatile("li a7, %1\n\t"
               "mv a0, %2\n\t"
               "mv a1, %3\n\t"
               "ecall\n\t"
               "mv %0, a0\n\t"
               : "=r"(ret)
               : "i"(__NR_kill), "r"(pid), "r"(sig)
               : "a0", "a1", "a7", "memory");
  return ret;
}

int raise(int sig) {
  return kill(getpid(), sig);
}

int sys_openat(int dfd, char *filename, int flags) {
    long syscall_ret;
    asm volatile ("li a7, %1\n"
                  "mv a0, %2\n"
                  "mv a1, %3\n"
                  "mv a2, %4\n"
                  "ecall\n"
                  "mv %0, a0\n"
                  : "+r" (syscall_ret)
                  : "i" (__NR_openat), "r" ((int64_t)dfd), "r" (filename), "r" ((int64_t)flags));
    return syscall_ret;
}

int open(char *filename, int flags) {
    return sys_openat(AT_FDCWD, filename, flags);
}

int close(int fd) {
    long syscall_ret;
    asm volatile ("li a7, %1\n"
                  "mv a0, %2\n"
                  "ecall\n"
                  "mv %0, a0\n"
                  : "+r" (syscall_ret)
                  : "i" (__NR_close), "r" ((int64_t)fd));
    return syscall_ret;
}

int lseek(int fd, int offset, int whence) {
    long syscall_ret;
    asm volatile ("li a7, %1\n"
                  "mv a0, %2\n"
                  "mv a1, %3\n"
                  "mv a2, %4\n"
                  "ecall\n"
                  "mv %0, a0\n"
                  : "+r" (syscall_ret)
                  : "i" (__NR_lseek), "r" ((int64_t)fd), "r" ((int64_t)offset), "r" ((int64_t)whence));
    return syscall_ret;
}