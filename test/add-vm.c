//#define DEBUG
#define STACKSIZE 16
#define NREGS 2

#include "vm.c"

static mem_t st[STACKSIZE], regs[NREGS];

#define PTRSIZE (sizeof (void*))
#define A 0
#define B 1

static const arg_t prog[] = {
    PUSH, PTRSIZE, PUSHARGV, ADD,
    DEREF, PTRSIZE,
    ATOI,
    COPY,
    PRINTN,

    PUSH, (arg_t)" + ",
    PRINTS,

    PUSH, PTRSIZE * 2, PUSHARGV, ADD,
    DEREF, PTRSIZE,
    ATOI,
    COPY,
    PRINTN,

    PUSH, (arg_t)" = ",
    PRINTS,

    ADD,
    PRINTN,

    PUSH, (arg_t)"\n",
    PRINTS,

    PUSH, 0, EXIT
};

int main(int argc, char **argv) {
    //return run_interpreter(prog, sizeof (prog) / sizeof (arg_t), 16, 0,
    //        argc, argv);
    const size_t progsize = sizeof (prog) / sizeof (arg_t);

    static size_t pc = 0, idx = 0;

#define STPUSH(val) (st[idx++] = (mem_t)(val))
#define STPOP (st[--idx])
#define NEXTARG (prog[++pc])

#ifdef DEBUG
#define DBG(msg) fprintf(stderr, "%-3u " msg "\n", pc)
#define DBGF(msg, args...) fprintf(stderr, "%-3u " msg "\n", pc, args)
#else
#define DBG(msg)
#define DBGF(...)
#endif

    while (pc < progsize) {
        switch ((cmd_t)prog[pc]) {
        case PUSH:
            {
            mem_t val = NEXTARG;
            DBGF("PUSH 0x%lx", val);
            STPUSH(val);
            }
            break;
        case POP:
            DBG("POP");
            (void)STPOP;
            break;
        case CMP:
            {
            op_t op = (op_t)NEXTARG;
            mem_t arg2 = STPOP;
            mem_t arg1 = STPOP;
            int result;
            switch (op) {
                case EQ: result = arg1 == arg2; break;
                //case LT: result = arg1 < arg2; break;
                case GT: result = arg1 > arg2; break;
                case LE: result = arg1 <= arg2; break;
                case GE: result = arg1 >= arg2; break;
                default: fprintf(stderr, "invalid op\n"); return -1;
            }
            DBGF("CMP 0x%lx %s 0x%lx  ; %d", arg1,
                    ((char*[]){"==", "<", ">", "<=", ">="})[op], arg2, result);
            STPUSH(result);
            }
            break;
        case DEREF:
            {
            size_t nbytes = NEXTARG;
            char* ptr = (char*)STPOP;
            DBGF("DEREF %zu %p", nbytes, ptr);
            mem_t val = 0;
            memcpy(&val, ptr, nbytes);
            DBGF("    ; value = 0x%lx", val);
            STPUSH(val);
            }
            break;
        case STORE:
            {
            size_t index = NEXTARG;
            mem_t val = STPOP;
            DBGF("STORE %zu 0x%lx", index, val);
            regs[index] = val;
            }
            break;
        case LOAD:
            {
            size_t index = NEXTARG;
            DBGF("LOAD %zu  ; 0x%lx", index, regs[index]);
            STPUSH(regs[index]);
            }
            break;
        case JUMP:
            {
            long offset = NEXTARG;
            DBGF("JUMP %ld", offset);
            pc += offset - 2;
            }
            break;
        case BRANCH:
            {
            long offset = NEXTARG;
            int condition = STPOP;
            DBGF("BRANCH %ld  ; %s", offset, condition ? "taken" : "not taken");
            if (condition)
                pc += offset - 2;
            }
            break;
        case STRLEN:
            {
            char* s = (char*)STPOP;
            DBG("STRLEN");
            DBGF("    ; str = \"%s\"", s);
            DBGF("    ; len = %zu", strlen(s));
            STPUSH(strlen(s));
            }
            break;
        case PUTS:
            {
            char* s = (char*)STPOP;
            DBG("PUTS");
            DBGF("    ; str = \"%s\"", s);
            puts(s);
            }
            break;
        case ADD:
            {
            long b = STPOP;
            long a = STPOP;
            DBGF("ADD %ld %ld  ; %ld", a, b, a + b);
            STPUSH(a + b);
            }
            break;
        case MUL:
            {
            long b = STPOP;
            long a = STPOP;
            DBGF("MUL %ld %ld  ; %ld", a, b, a * b);
            STPUSH(a * b);
            }
            break;
        case EXIT:
            {
            int status = STPOP;
            DBGF("EXIT %d", status);
            return status;
            }
            break;
        case SLEEP:
            {
            unsigned int seconds = STPOP;
            DBGF("SLEEP %u", seconds);
            sleep(seconds);
            }
            break;
        case PUSHARGC:
            {
            DBGF("PUSHARGC (%d)", argc);
            STPUSH(argc);
            }
            break;
        case PUSHARGV:
            {
            DBGF("PUSHARGV (%p)", argv);
            STPUSH(argv);
            }
            break;
        case ATOI:
            {
            char* s = (char*)STPOP;
            int i = atoi(s);
            DBGF("ATOI(\"%s\") = %d", s, i);
            STPUSH(i);
            }
            break;
        case PRINTN:
            {
            DBG("PRINTN");
            printf("%ld", STPOP);
            }
            break;
        case PRINTS:
            {
            DBG("PRINTS");
            printf("%s", (char*)STPOP);
            }
            break;
        case COPY:
            {
            DBG("COPY");
            arg_t x = STPOP;
            STPUSH(x);
            STPUSH(x);
            }
            break;
        }
        pc++;
    }

    DBGF("out of program bounds at pc %u", pc);
    return 1;
}
