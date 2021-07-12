#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PASSWORD "loltest"
//#define DEBUG

typedef enum {
    PUSH,   // <arg>  push value <arg>
    POP,    // discard value at the top of the stack
    CMP,    // pop <arg2> <arg1>  push <arg1> == <arg2>
    DEREF,  // <nbytes>  pop <ptr> <index>  push <nbytes> bytes from <ptr> + <index>
    STORE,  // <reg>  pop stack entry and save it to register <reg>
    LOAD,   // <reg>  push value in register <reg>
    JUMP,   // <offset>  jump <offset> cmds
    BRANCH, // <offset>  jump <offset> cmds if popped value is non-zero
    // CALL,      // <fn> <nargs> <ret>  pop <nargs> args, call <fn>(<args>), push result if <ret>
    ADD,    // pop <a> <b>  push <a> + <b>
    STRLEN, // pop <ptr>  push strlen(<ptr>)
    PUTS,   // pop <ptr>  call puts(<ptr>)
    EXIT,   // pop <status>  exit with status code <status>
    SLEEP,  // pop <n>  sleep for <n> seconds
} cmd_t;

typedef enum { EQ = 0, LT, GT, LE, GE } op_t;

typedef long arg_t;

typedef long mem_t;

static const size_t STACK_SIZE = 512;
static const size_t NREGS = 16;

#define PUSH1(cmd, arg) PUSH, (arg_t)(arg), cmd
#define PUSH2(cmd, arg1, arg2) PUSH, (arg_t)(arg2), PUSH, (arg_t)(arg1), cmd
#define FAIL_F(msg) BRANCH, 8, PUSH1(PUTS, msg), PUSH1(EXIT, 1)

int main(int argc, char **argv) {
    const char *const passwd = PASSWORD;

    arg_t prog[] = {// if (argc >= 2) exit(1)
                    PUSH, argc, PUSH, 2, CMP, GE, FAIL_F("missing password argument"),

                    // if (strlen(argv[1]) != strlen(passwd)) exit(1)
                    PUSH, sizeof(char *), PUSH, (arg_t)argv, DEREF, sizeof(char *), STRLEN, PUSH1(STRLEN, passwd),
#define REG_PASSWDLEN 0
                    STORE, REG_PASSWDLEN, LOAD, REG_PASSWDLEN, // passwd_len = strlen(passwd)
                    CMP, EQ, FAIL_F("wrong password length"),

    // compare argv[1] to passwd
#define REG_I 1
                    PUSH, 0, STORE, REG_I, // i = 0
                    // loopstart:
                    // if (i == passwd_len) goto loopend
                    LOAD, REG_I,         // 2 8
                    LOAD, REG_PASSWDLEN, // 2
                    CMP, EQ,             // 2
                    BRANCH, 37,          // 2 2

                    // push argv[1][i]
                    LOAD, REG_I,           // 2 10
                    PUSH, sizeof(char *),  // 2
                    PUSH, (arg_t)argv,     // 2
                    DEREF, sizeof(char *), // 2
                    DEREF, 1,              // 2

                    // push passwd[i]
                    LOAD, REG_I,         // 2 6
                    PUSH, (arg_t)passwd, // 2
                    DEREF, 1,            // 2

                    // if (argv[1][i] != passwd[i]) exit(1)
                    CMP, EQ,                      // 2 10
                    FAIL_F("incorrect password"), // 8

                    // i++
                    LOAD, REG_I,   // 2 7
                    PUSH1(ADD, 1), // 3
                    STORE, REG_I,  // 2

                    // goto loopstart
                    JUMP, -41, // 2 2
                               // loopend:
#undef REG_I
#undef REG_PASSWDLEN

                    // print success message
                    PUSH1(PUTS, "password is correct!"), PUSH1(SLEEP, 1), PUSH1(EXIT, 0)};

    mem_t st[STACK_SIZE], *stack = st;
    mem_t regs[NREGS];
    long pc = 0;

#define STPUSH(val) (*stack++ = (mem_t)(val))
#define STPOP (*--stack)
#define NEXTARG (prog[++pc])

#ifdef DEBUG
#define DBG(msg) fprintf(stderr, "%-3ld " msg "\n", pc)
#define DBGF(msg, args...) fprintf(stderr, "%-3ld " msg "\n", pc, args)
#else
#define DBG(msg)
#define DBGF(...)
#endif

    while (pc >= 0 && (size_t)pc < sizeof(prog)) {
        switch ((cmd_t)prog[pc]) {
        case PUSH: {
            mem_t val = NEXTARG;
            DBGF("PUSH 0x%lx", val);
            STPUSH(val);
        } break;
        case POP:
            DBG("POP");
            (void)STPOP;
            break;
        case CMP: {
            op_t op = (op_t)NEXTARG;
            mem_t arg2 = STPOP;
            mem_t arg1 = STPOP;
            int result;
            switch (op) {
            case EQ:
                result = arg1 == arg2;
                break;
            // case LT: result = arg1 < arg2; break;
            case GT:
                result = arg1 > arg2;
                break;
            case LE:
                result = arg1 <= arg2;
                break;
            case GE:
                result = arg1 >= arg2;
                break;
            default:
                fprintf(stderr, "invalid op\n");
                exit(-1);
            }
            DBGF("CMP 0x%lx %s 0x%lx  ; %d", arg1, ((char *[]){"==", "<", ">", "<=", ">="})[op], arg2, result);
            STPUSH(result);
        } break;
        case DEREF: {
            size_t nbytes = NEXTARG;
            char *ptr = (char *)STPOP;
            size_t index = STPOP;
            DBGF("DEREF %zu %p %zu", nbytes, ptr, index);
            mem_t val = 0;
            memcpy(&val, ptr + index, nbytes);
            DBGF("    ; value = 0x%lx", val);
            STPUSH(val);
        } break;
        case STORE: {
            size_t index = NEXTARG;
            mem_t val = STPOP;
            DBGF("STORE %zu 0x%lx", index, val);
            regs[index] = val;
        } break;
        case LOAD: {
            size_t index = NEXTARG;
            DBGF("LOAD %zu  ; 0x%lx", index, regs[index]);
            STPUSH(regs[index]);
        } break;
        case JUMP: {
            long offset = NEXTARG;
            DBGF("JUMP %ld", offset);
            pc += offset - 2;
        } break;
        case BRANCH: {
            long offset = NEXTARG;
            int condition = STPOP;
            DBGF("BRANCH %ld  ; %s", offset, condition ? "taken" : "not taken");
            if (condition)
                pc += offset - 2;
        } break;
        case STRLEN: {
            char *s = (char *)STPOP;
            DBG("STRLEN");
            DBGF("    ; str = \"%s\"", s);
            DBGF("    ; len = %zu", strlen(s));
            STPUSH(strlen(s));
        } break;
        case PUTS: {
            char *s = (char *)STPOP;
            DBG("PUTS");
            DBGF("    ; str = \"%s\"", s);
            puts(s);
        } break;
        case ADD: {
            long b = STPOP;
            long a = STPOP;
            DBGF("ADD %ld %ld  ; %ld", a, b, a + b);
            STPUSH(a + b);
        } break;
        case EXIT: {
            int status = STPOP;
            DBGF("EXIT %d", status);
            exit(status);
        } break;
        case SLEEP: {
            unsigned int seconds = STPOP;
            DBGF("SLEEP %u", seconds);
            sleep(seconds);
        } break;
        }
        pc++;
    }

    DBGF("out of program bounds at pc %ld", pc);
    return 1;
}
