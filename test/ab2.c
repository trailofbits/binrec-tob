#include <stdio.h>
#include "VirtualizerSDK.h"

void foo(char a, char b) {
    VIRTUALIZER_START;
    if (a == 'a' && b == 'b') {
        printf("You entered AB\n");
    } else if (a == 'a') {
        printf("You entered A_\n");
    } else if (b == 'b') {
        printf("You entered _B\n");
    }
    VIRTUALIZER_END;
}

int main(int argc, char **argv) {
    if (argc >= 2) {
        foo(argv[1][0], argv[1][1]);
    }
    return 0;
}
