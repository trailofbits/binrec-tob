//#define DEBUG
#define STACKSIZE 1024
#define NREGS 6

#include "vm.c"

#define CALL1(cmd, arg) PUSH, (arg_t)(arg), cmd
#define PTRSIZE (sizeof (void*))
#define INTARG(reg)     \
    PUSH, PTRSIZE, MUL, \
    PUSHARGV, ADD,      \
    DEREF, PTRSIZE,     \
    ATOI,               \
    STORE, reg
#define INC(reg, c) LOAD, reg, PUSH, c, ADD, STORE, reg
#define SET(reg, c) PUSH, c, STORE, reg

#define S 0
#define W 1
#define H 2
#define I 3
#define X 4
#define N 5

static const arg_t prog[] = {
    // if (argc < 4) goto end
    PUSHARGC, PUSH, 4, CMP, LT,
    BRANCH, 46,                         //  +2

    PUSH, 1, INTARG(S),                 // 12
    PUSH, 2, INTARG(W),                 // 12
    PUSH, 3, INTARG(H),                 // 12
    SET(I, 4),                          // 4
                                        //  +40
    // outer:
    // if (!h) goto end;
    LOAD, H,                            // 2
    BRANCH, 4,                          // 2
    JUMP, 65,                           // 2
    // h--;
    INC(H, -1),                         // 7
                                        //  +13
    // for (int x = 0; x < w; x++)
    SET(X, 0),                          // 4
    // inner:
    LOAD, X, LOAD, W, CMP, GE,          // 6
    BRANCH, 39,                         // 2
    // int n = atoi(argv[i++])          //  +8
    LOAD, I, INTARG(N),                 // 12
    INC(I, 1),                          // 7
    // printf("%d ", n * s)             //  +19
    LOAD, N, LOAD, S, MUL,              // 5
    PRINTN,                             // 1
    CALL1(PRINTS, " "),                 // 3
                                        //  +9
                                        //   ++40
    // goto inner;
    INC(X, 1),                          // 7
    JUMP, -43,                          // 2

    // goto outer
    CALL1(PRINTS, "\n"),             // 3
    JUMP, -65,                          // 2
                                        //  +5
    // end:
    // exit(0)
    CALL1(EXIT, 0)
};

int main(int argc, char **argv) {
    return run_interpreter(prog, sizeof (prog) / sizeof (arg_t), argc, argv);
}
