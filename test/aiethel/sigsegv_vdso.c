#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
//#include <unistd_32.h>

#define __NR_signal 48
typedef void (*signal_handler)(int);

//Use vdso to register a system call by calling __kernel_vsyscall.
//Address of __kernel_vsyscall is passed to user process with AT_SYSINFO elf parameter
//We make a call to %%gs:0x10 because when libc is loaded it copies the value of 
//AT_SYSINFO from the process stack to the TCB (Thread Control Block). Segment register
//%gs refers to the TCB. Offset 0x10 can be different in different systems.
signal_handler reg_signal(int signum, signal_handler handler) {
    signal_handler ret;
    __asm__ __volatile__
    (
        "call *%%gs:0x10"
        : "=a" (ret)
        : "0"(__NR_signal), "b"(signum), "c"(handler)
        : "cc", "edi", "esi", "memory"
    );
    return ret;
}

void sig_handler(int sig) {
    switch (sig) {
    case SIGSEGV:
        fprintf(stderr, "dumping everything..\n");
        exit(1);
    default:
        fprintf(stderr, "something caused a signal\n");
        exit(2);
    }
}

int main() {
    reg_signal(SIGSEGV, sig_handler);
    
    int *i_ptr = NULL;
    //trigger SIGSEGV
    return *i_ptr;
}
