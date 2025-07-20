#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <stddef.h>
#include <signal.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define AT_FDCWD    -100
#define O_RDONLY    0x0001
#define O_WRONLY    0x0002
#define O_RDWR      0x0003

#define SEEK_SET    0x0000
#define SEEK_CUR    0x0001
#define SEEK_END    0x0002

typedef int pid_t;
typedef long ssize_t;
// typedef long long intptr_t;

ssize_t write(int fd, const void *buf, size_t count);
pid_t getpid(void);
pid_t fork(void);
void *sbrk(long long increment);
ssize_t read(int fd, void *buf, size_t count);
int sigaction(int signum, const struct sigaction *restrict act, struct sigaction *restrict oldact);
sighandler_t signal(int signum, sighandler_t handler);
int kill(pid_t pid, int sig);
int raise(int sig);
int open(char *filename, int flags);
int close(int fd);
int lseek(int fd, int offset, int whence);

#endif
