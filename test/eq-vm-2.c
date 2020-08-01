//#define DEBUG
#define STACKSIZE 16
#define NREGS 0

#include "vm.c"

#define CALL1(cmd, arg) PUSH, (arg_t)(arg), cmd
#define DEREF_IDX(ptr, idx, nbytes) \
    PUSH, (idx), PUSH, (arg_t)(ptr), ADD, DEREF, (nbytes)
#define PTRSIZE (sizeof (void*))

static const arg_t prog[] = {
    // if (argc < 3) exit(1)
    PUSHARGC,
    PUSH, 3,
    CMP, GE,
    BRANCH, 8,                              // 2
    CALL1(PUTS, "usage: ./eq-vm-2 A B"),    // 3
    CALL1(EXIT, 1),                         // 3

    // if (argv[1][0] == argv[2][0]) { puts(msg1); goto end; }
    PUSH, PTRSIZE, PUSHARGV, ADD,
    DEREF, PTRSIZE,
    DEREF, 1,
    PUSH, PTRSIZE * 2, PUSHARGV, ADD,
    DEREF, PTRSIZE,
    DEREF, 1,
    CMP, EQ,
    BRANCH, 7,                              // 2
    CALL1(PUTS, "arguments are not equal"), // 3
    JUMP, 5,                                // 2

    // else puts(msg2)
    CALL1(PUTS, "arguments are equal"),     // 3

    // end:
    // exit(0)
    CALL1(EXIT, 0)
};

int main(int argc, char **argv) {
    return run_interpreter(prog, sizeof (prog) / sizeof (arg_t), argc, argv);
}
