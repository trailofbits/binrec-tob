#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd_32.h>

#define __NR_signal 48
typedef void (*func_0arg)();

// Use vdso to register a system call by calling __kernel_vsyscall.
// Address of __kernel_vsyscall is passed to user process with AT_SYSINFO elf parameter
// We make a call to %%gs:0x10 because when libc is loaded it copies the value of
// AT_SYSINFO from the process stack to the TCB (Thread Control Block). Segment register
//%gs refers to the TCB. Offset 0x10 can be different in different systems.
//        "call *%%0"
void execute(func_0arg f) { __asm__ __volatile__("calll *%0\n\t" : : "r"(f) :); }

void trash_work() {
    int i = 1;
    int j = 4;
    int k = i + j;
    puts("trash1");
    if (k)
        i = -i;
    else
        k = k + k;
    j--;
}

void trash_work2() {
    int i = 4;
    int j = 3;
    int k = i + j;
    puts("trash2");
    if (k)
        i = -i;
    else
        k = k * k + k;
    j--;
}

int main() {
    trash_work();
    trash_work2();

    return 0;
}
