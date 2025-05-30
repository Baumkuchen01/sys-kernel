#include <syscalls.h>
#include <unistd.h>
#include <stdint.h>

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
