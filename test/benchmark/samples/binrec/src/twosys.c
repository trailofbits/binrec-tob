#include <stdio.h>
#include <stdlib.h>

int main(void) {
    fprintf(stderr, "some error\n");
    void *mem = malloc(256);
    free(mem);
    fprintf(stderr, "second error\n");
    return 0;
}
