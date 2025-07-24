#ifndef __OPEN_H__
#define __OPEN_H__

int sys_open(const char *pathname, int flags);
int sys_openat(int dirfd, const char *pathname, int flags);
int sys_close(int fd);

#endif // __OPEN_H__