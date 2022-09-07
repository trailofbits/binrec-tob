#include "VirtualizerSDK.h"
#include <stdio.h>

void foo(char a) {
    VIRTUALIZER_START;
    if (a == 'a' || a == 'b') {
        printf("You entered an A or a B\n");
    }
    VIRTUALIZER_END;
}

int main(int argc, char **argv) {
    if (argc >= 2) {
        foo(argv[1][0]);
    }
    return 0;
}
