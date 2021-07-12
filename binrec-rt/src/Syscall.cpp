#include "binrec/rt/Syscall.h"
#include <sys/syscall.h>

namespace {
template <long name> inline auto doSyscall() -> long {
    long result;
    asm volatile("movl %1, %%eax\n\t"
                 "int $0x80\n\t"
                 : "=a"(result)
                 : "i"(name)
                 : "memory", "cc");
    return result;
}
template <long name> inline auto doSyscall(long arg1) -> long {
    long result;
    asm volatile("movl %1, %%eax\n\t"
                 "int $0x80\n\t"
                 : "=a"(result)
                 : "i"(name), "b"(arg1)
                 : "memory", "cc");
    return result;
}
template <long name> inline auto doSyscall(long arg1, long arg2) -> long {
    long result;
    asm volatile("movl %1, %%eax\n\t"
                 "int $0x80\n\t"
                 : "=a"(result)
                 : "i"(name), "b"(arg1), "c"(arg2)
                 : "memory", "cc");
    return result;
}
template <long name> inline auto doSyscall(long arg1, long arg2, long arg3) -> long {
    long result;
    asm volatile("movl %1, %%eax\n\t"
                 "int $0x80\n\t"
                 : "=a"(result)
                 : "i"(name), "b"(arg1), "c"(arg2), "d"(arg3)
                 : "memory", "cc");
    return result;
}
template <long name> inline auto doSyscall(long arg1, long arg2, long arg3, long arg4) -> long {
    long result;
    asm volatile("movl %1, %%eax\n\t"
                 "int $0x80\n\t"
                 : "=a"(result)
                 : "i"(name), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4)
                 : "memory", "cc");
    return result;
}
template <long name> inline auto doSyscall(long arg1, long arg2, long arg3, long arg4, long arg5) -> long {
    long result;
    asm volatile("movl %1, %%eax\n\t"
                 "int $0x80\n\t"
                 : "=a"(result)
                 : "i"(name), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4)
                 : "memory", "cc");
    return result;
}
template <long name> inline auto doSyscall(long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) -> long {
    long result;
    asm volatile("pushl %%ebp\n\t"
                 "movl %7, %%ebp\n\t"
                 "movl %1, %%eax\n\t"
                 "int $0x80\n\t"
                 "popl %%ebp\n\t"
                 : "=a"(result)
                 : "i"(name), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5), "g"(arg6)
                 : "memory", "cc");
    return result;
}
} // namespace

void binrec_rt_exit(int status) { doSyscall<SYS_exit>(status); }

auto binrec_rt_close(int fd) -> int { return doSyscall<SYS_close>(fd); }

auto binrec_rt_open(const char *pathname, int flags) -> int {
    return doSyscall<SYS_open>(reinterpret_cast<long>(pathname), flags, 0664);
}

auto binrec_rt_write(int fd, const void *buf, size_t count) -> ssize_t {
    return doSyscall<SYS_write>(fd, reinterpret_cast<long>(buf), count);
}

auto binrec_rt_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) -> void * {
    return reinterpret_cast<void *>(
        doSyscall<SYS_mmap2>(reinterpret_cast<long>(addr), length, prot, flags, fd, offset / 4096ULL));
}

auto binrec_rt_munmap(void *addr, size_t length) -> int {
    return doSyscall<SYS_munmap>(reinterpret_cast<long>(addr), length);
}
