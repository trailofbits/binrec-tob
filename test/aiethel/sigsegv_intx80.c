#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd_32.h>

#define __NR_signal 48
typedef void (*signal_handler)(int);

// Implement signal system call with interrupt
signal_handler reg_signal(int signum, signal_handler handler) {
    signal_handler ret;
    __asm__ __volatile__("int $0x80"
                         : "=a"(ret)
                         : "0"(__NR_signal), "b"(signum), "c"(handler)
                         : "cc", "edi", "esi", "memory");
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
    // trigger SIGSEGV
    return *i_ptr;
}
