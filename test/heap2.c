#include "stdlib.h"
// known fail with asan
// binrec asan catches
int main(int argc, char **argv) {

    char *array = (char *)malloc(100);
    array[0] = '\0';
    int res = array[argc + 100]; // BOOM
    free(array);
    return res;
}
