#ifndef BINREC_SYSCALL_H
#define BINREC_SYSCALL_H

#ifdef __cplusplus
extern "C" {
#endif
#include <sys/mman.h>
#include <unistd.h>

void binrec_rt_exit(int status);
int binrec_rt_close(int fd);
int binrec_rt_open(const char* pathname, int flags);
ssize_t binrec_rt_write(int fd, const void *buf, size_t count);
void *binrec_rt_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int binrec_rt_munmap(void *addr, size_t length);
#ifdef __cplusplus
}
#endif

#endif
