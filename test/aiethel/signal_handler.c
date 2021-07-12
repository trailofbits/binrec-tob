#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void sig_handler(int);

int main() {
    signal(SIGABRT, sig_handler);
    signal(SIGALRM, sig_handler);
    signal(SIGSEGV, sig_handler);

    int *p = NULL;
    // trigger SIGSEGV
    int i = *p;
    (void)(i);
}

void sig_handler(int sig) {
    switch (sig) {
    case SIGSEGV:
        fprintf(stderr, "invalid dereference..dump everrrything...\n");
        abort();
    default:
        fprintf(stderr, "you gotta be kiding me!\n");
        abort();
    }
}
/*
#include<signal.h>
#include<stdio.h>
#include<wait.h>

// Handler for SIGINT, caused by
// Ctrl-C at keyboard
void handle_sigint(int sig)
{
    printf("Caught signal %d\n", sig);
}

int main()
{
    signal(SIGINT, handle_sigint);
    exit(0);
    return 0;
} */
