#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static int GlobalInt = 10;
static double GlobalDouble = 8.2;

static double get_double() {
    return GlobalDouble;
}

static double get_double_times2() {
    return GlobalDouble * 2.0;
}

static int get_int() {
    return GlobalInt;
}

static int get_int_times2() {
    return GlobalInt * 2;
}

static double double_times2(double x) {
    return x * 2.0;
}

static int int_times2(int x) {
    return x * 2;
}

static double double_plus2(double x) {
    return x + 2.0;
}

static int int_plus2(int x) {
    return x + 2;
}

int main(int argc, char **argv) {
    printf("get_int(): %d\n", get_int());
    printf("get_int_times2(): %d\n", get_int_times2());
    printf("int_times2([global] %d): %d\n", GlobalInt, int_times2(GlobalInt));
    printf("plus2([global] %d): %d\n", GlobalInt, int_plus2(GlobalInt));

    printf("\n");

    printf("get_double(): %f\n", get_double());
    printf("get_double_times2(): %f\n", get_double_times2());
    printf("double_times2([global] %f): %f\n", GlobalDouble, double_times2(GlobalDouble));
    printf("double_plus2([global] %f): %f\n", GlobalDouble, double_plus2(GlobalDouble));

    return 0;
}
