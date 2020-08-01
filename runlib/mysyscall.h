#ifndef _MYSYSCALL_H
#define _MYSYSCALL_H

#define SYSCALL0(nr)         \
    __asm__ __volatile__ (   \
        "movl %0, %%eax\n\t" \
        "int $0x80"          \
        :: "g"(nr)           \
        : "%eax"             \
    )

#define SYSCALL1(nr, arg1)    \
    __asm__ __volatile__ (    \
        "movl %0, %%eax\n\t"  \
        "movl %1, %%ebx\n\t"  \
        "int $0x80"           \
        :: "g"(nr), "g"(arg1) \
        : "%eax", "%ebx"      \
    )

#define SYSCALL2(nr, arg1, arg2)         \
    __asm__ __volatile__ (               \
        "movl %0, %%eax\n\t"             \
        "movl %1, %%ebx\n\t"             \
        "movl %2, %%ecx\n\t"             \
        "int $0x80"                      \
        :: "g"(nr), "g"(arg1), "g"(arg2) \
        : "%eax", "%ebx", "%ecx"         \
    )

#define SYSCALL3(nr, arg1, arg2, arg3)              \
    __asm__ __volatile__ (                          \
        "movl %0, %%eax\n\t"                        \
        "movl %1, %%ebx\n\t"                        \
        "movl %2, %%ecx\n\t"                        \
        "movl %3, %%edx\n\t"                        \
        "int $0x80"                                 \
        :: "g"(nr), "g"(arg1), "g"(arg2), "g"(arg3) \
        : "%eax", "%ebx", "%ecx", "%edx"            \
    )

#define SYSCALL4(nr, arg1, arg2, arg3, arg4)                   \
    __asm__ __volatile__ (                                     \
        "movl %0, %%eax\n\t"                                   \
        "movl %1, %%ebx\n\t"                                   \
        "movl %2, %%ecx\n\t"                                   \
        "movl %3, %%edx\n\t"                                   \
        "movl %4, %%esi\n\t"                                   \
        "int $0x80"                                            \
        :: "g"(nr), "g"(arg1), "g"(arg2), "g"(arg3), "g"(arg4) \
        : "%eax", "%ebx", "%ecx", "%edx", "%esi"               \
    )

#define SYSCALL5(nr, arg1, arg2, arg3, arg4, arg5)                        \
    __asm__ __volatile__ (                                                \
        "movl %0, %%eax\n\t"                                              \
        "movl %1, %%ebx\n\t"                                              \
        "movl %2, %%ecx\n\t"                                              \
        "movl %3, %%edx\n\t"                                              \
        "movl %4, %%esi\n\t"                                              \
        "movl %5, %%edi\n\t"                                              \
        "int $0x80"                                                       \
        :: "g"(nr), "g"(arg1), "g"(arg2), "g"(arg3), "g"(arg4), "g"(arg5) \
        : "%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi"                  \
    )

#define SYSCALL_RET (ssize_t)({ ssize_t r; __asm__ ("" : "=a"(r) ::); r; })

#endif  // _MYSYSCALL_H
