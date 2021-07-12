#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 2)
        return 1;

    float a = strtof(argv[1], NULL);
    printf("%f, %lx\n", a, *(long unsigned *)&a);

    return 0;
}
