#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc >= 4) {
        int s = atoi(argv[1]);
        int w = atoi(argv[2]);
        int h = atoi(argv[3]);
        int i = 4;
        while (h--) {
            for (int x = 0; x < w; x++) {
                int n = atoi(argv[i++]);
                printf("%d ", n * s);
            }
            printf("\n");
        }
    }
    return 0;
}
