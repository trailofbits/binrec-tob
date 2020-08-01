//#define DEBUG
#define STACKSIZE 16
#define NREGS 0

#include "vm.c"

#define CALL1(cmd, arg) PUSH, (arg_t)(arg), cmd
#define PTRSIZE (sizeof (void*))

static const arg_t prog[] = {
    // if (argc != 3)
    //   goto end
    PUSHARGC,
    PUSH, 3,
    CMP, NE,
    BRANCH, 25,                         // 2

    // if (argv[1][0] != argv[2][0])
    //   goto end
    PUSH, PTRSIZE, PUSHARGV, ADD,       // 4
    DEREF, PTRSIZE,                     // 2
    DEREF, 1,                           // 2
    PUSH, PTRSIZE * 2, PUSHARGV, ADD,   // 4
    DEREF, PTRSIZE,                     // 2
    DEREF, 1,                           // 2
    CMP, NE,                            // 2
    BRANCH, 7,                          // 2

    // puts(msg)
    CALL1(PUTS, "arguments are equal"), // 3

    // end:
    // exit(0)
    CALL1(EXIT, 0)
};

int main(int argc, char **argv) {
    return run_interpreter(prog, sizeof (prog) / sizeof (arg_t), argc, argv);
}
