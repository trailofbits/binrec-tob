#include <stdio.h>

void my_int_func(int x) { printf("%d\n", x); }
void my_int_func2(int x) { printf("bar\n"); }

int main(int argc, char *argv[]) {
    // make an indirect branch where one side is not recovered, so fallback is invoked
    //- select branch with a switch at runtime
    //- print address, enter address
    //- add many possible targets
    //- add loops, to confuse symbolic execution
    //
    // make fallback so inner function returns
    //- inline that innner function in recovered code
    //
    void (*foo)(int);
    void (*bar)(int);
    foo = &my_int_func;
    bar = &my_int_func2;
    int dist = bar - foo;
    long count = atol(argv[1]);
    printf("%lu\n", count);
    /*
    while(count > 0){
        count--;
    }
    */
    if (count == 42) {
        foo += dist;
    }
    foo(2);
    return 0;
}
