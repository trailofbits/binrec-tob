#ifndef _VM_H
#define _VM_H

typedef enum {
    PUSH,       // <arg>  push value <arg>
    POP,        // discard value at the top of the stack
    CMP,        // pop <arg2> <arg1>  push <arg1> <op> <arg2>
    DEREF,      // <nbytes>  pop <ptr>  push <nbytes> bytes from <ptr>
    STORE,      // <reg>  pop stack entry and save it to register <reg>
    LOAD,       // <reg>  push value in register <reg>
    JUMP,       // <offset>  jump <offset> cmds
    BRANCH,     // <offset>  jump <offset> cmds if popped value is non-zero
    ADD,        // pop <a> <b>  push <a> + <b>
    MUL,        // pop <a> <b>  push <a> * <b>
    STRLEN,     // pop <ptr>  push strlen(<ptr>)
    PUTS,       // pop <ptr>  call puts(<ptr>)
    EXIT,       // pop <status>  exit with status code <status>
    SLEEP,      // pop <n>  sleep for <n> seconds
    PUSHARGC,   // push argc
    PUSHARGV,   // push argv
    ATOI,
    PRINTN,
    PRINTS,
    COPY
} cmd_t;

typedef enum { EQ = 0, NE, LT, GT, LE, GE } op_t;

typedef long arg_t;

typedef long mem_t;

//int run_interpreter(const arg_t *prog, const size_t progsize,
//        const size_t stacksize, const size_t nregs,
//        int argc, char **argv);

#endif // _VM_H
