#define _SVID_SOURCE
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 3)
        return -1;

    double f = atof(argv[1]);
    int ndigits = atoi(argv[2]);
    int decpt, sign;
    char *s = fcvt(f, ndigits, &decpt, &sign);

    if (sign)
        putchar('-');

    for (char *c = s; *c; c++) {
        if (c == s + decpt)
            putchar('.');
        putchar(*c);
    }

    putchar('\n');

    return 0;
}
