#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void sig_handler(int);

int main() {
    signal(SIGABRT, sig_handler);
    //send abort signal
    abort();
}

void sig_handler(int sig) {
    switch (sig) {
    case SIGABRT:
        fprintf(stderr, "about to end..\n");
        exit(1);
    case SIGALRM:
        fprintf(stderr, "time is up!\n");
    case SIGSEGV:
        fprintf(stderr, "dumping everything..\n");
        abort();
    default:
        fprintf(stderr, "something caused a signal\n");
        abort();
    }
}
