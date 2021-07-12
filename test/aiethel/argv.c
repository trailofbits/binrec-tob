#include <stdio.h>

int i = 1;
int main(int argc, char *argv[]) {
    //    int i = 1;
    printf("%p\n", (void *)&i);
    printf("%d\n", argc);
    for (i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
        printf("%p\n", (void *)argv[i]);
    }
    return 0;
}
