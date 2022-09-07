#include <stdio.h>
// known violation with asan, binrec asan does not catch
int main(int argc, char **argv) {
    int stk[100];
    stk[1] = 0;
    puts("why\n");
    return stk[argc + 100];
}
