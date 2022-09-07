#include <stdio.h>

int mul4(const int na[10]) {
    int prod = 1;
    for (int i = 0; i < 10; ++i) {
        prod *= na[i];
    }
    return prod;
}

int foo(int x) { return 2 * x; }

int main(void) {
    puts("Hello world!");
    return 0;
}
