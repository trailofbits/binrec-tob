#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc == 3) {
        char a = argv[1][0];
        char b = argv[2][0];
        if (a == b) {
            puts("arguments are equal");
        } else {
            puts("arguments are NOT equal");
        }
    }
    return 0;
}
