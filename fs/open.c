#include <open.h>
#include <fs.h>
#include <fcntl.h>
#include <proc.h>
#include <string.h>
#include <printk.h>

extern struct task_struct *current;

int get_unused_fd() {
    struct files_struct *files = current->files;
    for (int i = 0; i < MAX_FILE_NUMBER; i++) {
        if (files->fd_array[i].opened == 0) {
            return i;
        }
    }
    return -1;
}

int sys_openat(int dirfd, const char *pathname, int flags) {
    if (dirfd != AT_FDCWD) {
        printk("openat: dirfd != AT_FDCWD is not supported\n");
        return -1;
    }
    return sys_open(pathname, flags);
}

int sys_open(const char *pathname, int flags) {
    int fd = get_unused_fd();
    if (fd < 0) {
        printk("No available file descriptor\n");
        return -1;
    }

    char kpathname[MAX_PATH_LENGTH];
    strcpy(kpathname, pathname);

    struct file *file = &(current->files->fd_array[fd]);
    if (file_open(file, kpathname, flags) < 0) {
        return -1;
    }
    return fd;
}

int sys_close(int fd) {
    if (fd < 0 || fd >= MAX_FILE_NUMBER) {
        printk("Invalid file descriptor: %d\n", fd);
        return -1;
    }

    struct files_struct *files = current->files;
    if (!files->fd_array[fd].opened) {
        printk("File descriptor %d is not opened\n", fd);
        return -1;
    }

    files->fd_array[fd].opened = 0;
    files->fd_array[fd].perms = 0;
    files->fd_array[fd].cfo = 0;
    files->fd_array[fd].lseek = NULL;
    files->fd_array[fd].write = NULL;
    files->fd_array[fd].read = NULL;

    return 0;
}