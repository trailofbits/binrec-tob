//#define DEBUG
#define STACKSIZE 64
#define NREGS 4

#include "vm.h"

#define CALL1(cmd, arg) PUSH, (arg_t)(arg), cmd
#define PTRSIZE (sizeof(void *))

#define N 0
#define X 1
#define Y 2
#define Z 3

static const arg_t prog[] = {
    // if (argc != 2)
    //   goto end
    PUSHARGC, PUSH, 2, CMP, NE, BRANCH, 59, // +2

    // int n = atoi(argv[1]);
    PUSH, PTRSIZE, PUSHARGV, ADD, // 4
    DEREF, PTRSIZE,               // 2
    ATOI,                         // 1
    STORE, N,                     // 2
                                  // +9
    // int x = 0, y = 1, z = 1, i = 0;
    PUSH, 0, STORE, X, // 4
    PUSH, 1, STORE, Y, // 4
    PUSH, 1, STORE, Z, // 4
                       // +12
    // loop:
    // if (!n) goto print;
    LOAD, N,   // 2
    BRANCH, 4, // 2
    JUMP, 26,  // 2
    // n--;
    LOAD, N, PUSH, -1, ADD, STORE, N, // 7
    // x = y;
    LOAD, Y, STORE, X, // 4
    // y = z;
    LOAD, Z, STORE, Y, // 4
    // z = x + y;
    LOAD, X, LOAD, Y, ADD, STORE, Z, // 7
    // goto loop;
    JUMP, -28, // 2
               // +28
    // print:
    // printn(x); prints("\n");
    LOAD, X,             // 2
    PRINTN,              // 1
    CALL1(PRINTS, "\n"), // 3
                         // +6
    // end:
    // exit(0)
    CALL1(EXIT, 0)};

int main(int argc, char **argv) { return run_interpreter(prog, sizeof(prog) / sizeof(arg_t), argc, argv); }
