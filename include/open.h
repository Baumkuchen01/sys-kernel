#ifndef __OPEN_H__
#define __OPEN_H__

long sys_open(const char *pathname, int flags);
long sys_openat(int dirfd, const char *pathname, int flags);
long sys_close(int fd);
long sys_lseek(int fd, int offset, int whence);

#endif // __OPEN_H__