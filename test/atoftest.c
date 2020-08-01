#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 3)
        return 1;

    double a = atof(argv[1]);
    printf("a: %f, %llx\n", a, *(long long unsigned*)&a);

    double b = atof(argv[2]);
    printf("b: %f, %llx\n", b , *(long long unsigned*)&b);

    double c = a * b;
    printf("a * b: %f, %llx\n", c, *(long long unsigned*)&c);

    return 0;
}
