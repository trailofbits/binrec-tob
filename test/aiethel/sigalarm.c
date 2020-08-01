#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void sig_handler(int);

int main() {
    signal(SIGALRM, sig_handler);
    //send SIGALRM after one sec
    alarm(1);
    //pause process until a signal is received.
    pause();
}

void sig_handler(int sig) {
    switch (sig) {
    case SIGABRT:
        fprintf(stderr, "about to end..\n");
        exit(1);
    case SIGALRM:
        fprintf(stderr, "time is up!\n");
        exit(0);
    case SIGSEGV:
        fprintf(stderr, "dumping everything..\n");
        abort();
    default:
        fprintf(stderr, "something caused a signal\n");
        abort();
    }
}
/*
#include<stdio.h> 
#include<signal.h> 
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
