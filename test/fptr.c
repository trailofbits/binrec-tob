#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void a() { puts("a, i<4"); }
size_t b(const char *fu) {
    puts("b");
    return 5;
}
size_t (*p[2])(const char *);
int main(int argc, char **argv) {
    p[0] = strlen;
    p[1] = b;
    char d = argv[1][0];
    (void)(d);
    char e = argv[2][0];
    if (!(e - '0')) {
        puts("yay");
    }
    for (int i = 0; i < 2; i++) {

        (*p[i + e - '0'])("hello");
    }
    puts("fin");
    return 0;
}
