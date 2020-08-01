#include <stdio.h>
#include <stdlib.h>
#include "VirtualizerSDK.h"

int fib(int n) {
    VIRTUALIZER_START;
    return n > 1 ? n * fib(n - 1) : n;
    VIRTUALIZER_END;
}

int main(int argc, char **argv) {
    if (argc >= 2) {
        int n = atoi(argv[1]);
        printf("fib(%d) = %d\n", n, fib(n));
    }
    return 0;
}
