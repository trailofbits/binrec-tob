#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 3)
        return 1;

    long long ai = atoll(argv[1]);
    double a = (double)ai;
    printf("a: %lld, %f, %llx\n", ai, a, *(long long unsigned*)&a);

    long long bi = atoll(argv[2]);
    double b = (double)bi;
    printf("b: %lld, %f, %llx\n", bi, b, *(long long unsigned*)&b);

    double c = a + b;
    printf("a + b: %lld, %f, %llx\n", ai + bi, c, *(long long unsigned*)&c);

    return 0;
}
