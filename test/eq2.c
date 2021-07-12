#include <stdio.h>
#include <stdlib.h>

void B() { puts("B"); }

void A() { puts("A"); }

int main(int argc, char **argv) {
    char a = argv[1][0];
    char b = argv[2][0];
    if (a == b) {
        puts("arguments are equal");
        A();
    } else {
        puts("arguments are NOT equal");
        B();
    }
    return 0;
}
