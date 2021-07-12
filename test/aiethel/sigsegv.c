#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
    signal(SIGSEGV, sig_handler);

    int *i_ptr = NULL;
    // trigger SIGSEGV
    return *i_ptr;
}
