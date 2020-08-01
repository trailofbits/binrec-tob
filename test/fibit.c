#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc >= 2) {
        int n = atoi(argv[1]);
        int x = 0, y = 1, z = 1;
        while (n--) {
            x = y;
            y = z;
            z = x + y;
        }
        printf("%d\n", x);
    }
    return 0;
}
