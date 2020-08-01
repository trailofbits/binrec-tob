#include <sys/syscall.h>
#include <sys/types.h>
#include <alloca.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <asm/ldt.h>

#include "cpu.h"
#include "softfloat.h"
#include "mysyscall.h"

#define DEBUG_FD STDERR_FILENO

typedef target_ulong addr_t;
typedef addr_t stackword_t;
typedef uint32_t reg_t;

// defined in llvm module
extern uint32_t debug_verbosity;

extern uint32_t PC;

#undef R_EAX
#undef R_EBX
#undef R_ECX
#undef R_EDX
#undef R_ESI
#undef R_EDI
#undef R_ESP
#undef R_EBP
extern reg_t R_EAX, R_EBX, R_ECX, R_EDX, R_ESI, R_EDI, R_EBP, R_ESP;

extern int32_t df;
extern uint32_t cc_src, cc_dst, cc_op, mflags;

extern SegmentCache segs[6];

extern float_status fp_status;
extern unsigned int fpstt;
extern FPReg fpregs[8];
extern uint8_t fptags[8];

extern void wrapper(void);
//This comes with captured.bc. Originially implemented in op_helper.c. op_helper.bc is already linked with captured.bc in s2e.
extern void helper_fninit(void); 

// copied from s2e/qemu/target-i386/op_helpers.c
const uint8_t parity_table[256] = {
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    CC_P, 0, 0, CC_P, 0, CC_P, CC_P, 0,
    0, CC_P, CC_P, 0, CC_P, 0, 0, CC_P,
};

#define OUTPUT(msg) {              \
        static const char buf[] = msg; \
        mywrite(DEBUG_FD, buf, sizeof (buf) - 1);  \
    }

typedef unsigned uint;

static ssize_t uint_nchars(uint i, uint base) {
    uint rem = i;
    ssize_t nchars = 0;

    do {
        nchars++;
        rem /= base;
    } while (rem);

    return nchars;
}

ssize_t __attribute__((noinline)) mywrite(int fd, const void *buf, size_t count) {
    SYSCALL3(SYS_write, fd, buf, count);
    return SYSCALL_RET;
}

static inline void blit_char(char *buf, char c, ssize_t count) {
    while (count--) {
        asm ("nop");  // Prevent llvm-opt from creating a memset intrinsic
        *buf++ = c;
    }
}

ssize_t __attribute__((noinline)) mywrite_chars(int fd, char c, ssize_t count) {
    char *buf = alloca(count);
    blit_char(buf, c, count);
    return mywrite(fd, buf, count);
}

ssize_t __attribute__((noinline)) mywrite_char(int fd, char c) {
    return mywrite_chars(fd, c, 1);
}

static ssize_t uint64_nchars(uint64_t i, uint base) {
    uint64_t rem = i;
    ssize_t nchars = 0;

    do {
        nchars++;
        rem /= base;
    } while (rem);

    return nchars;
}
static void blit_uint64(char *buf, uint64_t i, uint base) {
    static const char digits[] = "0123456789abcdef";
    buf += uint64_nchars(i, base);

    do {
        *--buf = digits[i % base];
        i /= base;
    } while (i);
}
ssize_t __attribute__((noinline)) mywrite_int64(int fd, int64_t i, uint base) {
    int sign = i < 0;
    uint64_t j = sign ? -i : i;
    ssize_t nchars = sign + uint_nchars(j, base);
    char *buf = alloca(nchars);
    if (sign) *buf = '-';
    blit_uint64(buf + sign, j, base);
    return mywrite(fd, buf, nchars);
}
ssize_t __attribute__((noinline)) mywrite_uint64(int fd, uint64_t i, uint base) {
    ssize_t nchars = uint64_nchars(i, base);
    char *buf = alloca(nchars);
    blit_uint64(buf, i, base);
    return mywrite(fd, buf, nchars);
}

static void blit_uint(char *buf, uint i, uint base) {
    static const char digits[] = "0123456789abcdef";
    buf += uint_nchars(i, base);

    do {
        *--buf = digits[i % base];
        i /= base;
    } while (i);
}

ssize_t __attribute__((noinline)) mywrite_int(int fd, int i, uint base) {
    int sign = i < 0;
    uint j = sign ? -i : i;
    ssize_t nchars = sign + uint_nchars(j, base);
    char *buf = alloca(nchars);
    if (sign) *buf = '-';
    blit_uint(buf + sign, j, base);
    return mywrite(fd, buf, nchars);
}

ssize_t __attribute__((noinline)) mywrite_uint(int fd, uint i, uint base) {
    ssize_t nchars = uint_nchars(i, base);
    char *buf = alloca(nchars);
    blit_uint(buf, i, base);
    return mywrite(fd, buf, nchars);
}

ssize_t __attribute__((noinline)) mywrite_bin(int fd, uint i) {
    return mywrite_uint(fd, i, 2);
}

ssize_t __attribute__((noinline)) mywrite_oct(int fd, uint i) {
    return mywrite_uint(fd, i, 8);
}

ssize_t __attribute__((noinline)) mywrite_dec(int fd, int i) {
    return mywrite_int(fd, i, 10);
}

ssize_t __attribute__((noinline)) mywrite_hex(int fd, uint i) {
    return mywrite_uint(fd, i, 16);
}

ssize_t __attribute__((noinline)) mywrite_uint_pad(int fd, uint i, uint base, char c, ssize_t width) {
    ssize_t nchars = uint_nchars(i, base);
    ssize_t pad = nchars < width ? width - nchars : 0;
    char *buf = alloca(pad + nchars);
    blit_char(buf, c, pad);
    blit_uint(buf + pad, i, base);
    return mywrite(fd, buf, pad + nchars);
}

ssize_t __attribute__((noinline)) pad_right(int fd, ssize_t nwritten, char c, ssize_t width) {
    return width > nwritten ? mywrite_chars(fd, c, width - nwritten) : 0;
}

void __attribute__((noinline)) myexit(int status) {
    SYSCALL1(SYS_exit, status);
}

void raise_interrupt(int intno, int is_int, int error_code,
        int next_eip_addend) {
/*    OUTPUT("interrupt intno=");
    mywrite_dec(DEBUG_FD, intno);
    OUTPUT(" is_int=");
    mywrite_dec(DEBUG_FD, is_int);
    OUTPUT(" error_code=");
    mywrite_dec(DEBUG_FD, error_code);
    OUTPUT(" next_eip_addend=");
    mywrite_hex(DEBUG_FD, next_eip_addend);
    mywrite_char(DEBUG_FD, '\n');
*/    myexit(-1);
}

void __attribute__((noinline)) helper_raise_exception(uint32_t index) {
    OUTPUT("exception ");
    mywrite_dec(DEBUG_FD, index);
    OUTPUT(" raised\n");
    myexit(-1);
}

static const size_t STACK_SIZE = (1024 * 1024 * 16) / sizeof(stackword_t);  // 16MB
static stackword_t stack[STACK_SIZE] __attribute__((aligned(16)));
static char segmem[1024];

static uint16_t get_gs() {
    uint16_t gs;
    asm volatile ("mov %%gs, %0" : "=g"(gs) ::);
    return gs;
}

static void init_gs(SegmentCache *ds) {
    //ds->selector = get_gs();
    //SYSCALL1(SYS_get_thread_area, (struct user_desc*)ds);
    //struct user_desc info = {.entry_number = get_gs()};
    //SYSCALL1(SYS_get_thread_area, &info);

    //ssize_t ret = SYSCALL_RET;  // FIXME: THIS KEEPS FAILING!!!???
    //if (ret) {
    //    OUTPUT("get_thread_area returned ");
    //    mywrite_dec(DEBUG_FD, ret);
    //    mywrite_char(DEBUG_FD, '\n');
    //    myexit(-2);
    //}

    //ds->base = info.base_addr;

    // for now just emulate the TLS by setting the base pointer to a global
    // array so that accesses do not segfault (note that the stack canary will
    // always be zero)
    ds->base = (target_ulong)segmem;
}

static void init_env() {
    reg_t eflags;
    asm (
        "pushf\n\t"
        "popl %0"
        : "=r"(eflags) ::
    );

    mflags = eflags & MFLAGS_MASK;
    df = eflags & DF_MASK ? -1 : 1;
}

//void __attribute__((noinline)) helper_debug_state();

int main(int argc, char **argv, char **envp) {
    assert(sizeof (stackword_t) == sizeof (void*));

    if (debug_verbosity >= 1){
        OUTPUT("stack-begins at = ");
        mywrite_hex(DEBUG_FD, (unsigned long) &stack[STACK_SIZE - 1]);
        mywrite_char(DEBUG_FD, '\n');
        OUTPUT("start of custom main\n");
    }

    init_env();
    init_gs(&segs[R_GS]);
    helper_fninit();

    stack[STACK_SIZE - 1] = (stackword_t)envp;
    stack[STACK_SIZE - 2] = (stackword_t)argv;
    stack[STACK_SIZE - 3] = (stackword_t)argc;
    stack[STACK_SIZE - 4] = (stackword_t)__builtin_return_address(0);

    R_ESP = (stackword_t)&stack[STACK_SIZE - 4];

    //helper_debug_state();
    wrapper();

    if (debug_verbosity >= 1)
        OUTPUT("end of custom main\n");

    return (int)R_EAX;
}

void  __attribute__((always_inline)) nonlib_setjmp(){

    addr_t retaddr = __ldl_mmu(R_ESP, 0);
    R_ESP += sizeof (stackword_t);

    reg_t *r_esp = (reg_t *)R_ESP;
    unsigned long *buf = (unsigned long *)*r_esp;
    buf[0] = R_EBX;
    buf[1] = R_ESI;
    buf[2] = R_EDI;
    buf[3] = R_EBP;
    buf[4] = R_ESP;
    buf[5] = retaddr;
    R_EAX = 0;
    PC = retaddr;
}

void  __attribute__((always_inline)) nonlib_longjmp(){

    //pop ret address, we don't need it
    //addr_t retaddr = __ldl_mmu(R_ESP, 0);
    R_ESP += sizeof (stackword_t);
    //get first param(buffer)
    reg_t *r_esp = (reg_t *)R_ESP;
    unsigned long *buf = (unsigned long *)*r_esp;
    //get second param(ret val for setjmp)
    R_ESP += sizeof (stackword_t);
    r_esp = (reg_t *)R_ESP;
    R_EAX = *r_esp;

    if(!R_EAX)
        R_EAX++;
    R_EBX = buf[0];
    R_ESI = buf[1];
    R_EDI = buf[2];
    R_EBP = buf[3];
    R_ESP = buf[4];
    PC = buf[5];
}

uint64_t __attribute__((noinline, fastcall)) helper_stub_trampoline(
        const reg_t edx, const reg_t ecx, const reg_t esp,
        const addr_t targetpc) {
    // see https://developer.apple.com/library/mac/documentation/DeveloperTools/Conceptual/LowLevelABI/130-IA-32_Function_Calling_Conventions/IA32.html

    // This struct is optimized away by the compiler because the return value
    // of the 'call' below is returned in edx/eax and then also used as return
    // value for this helper. Its purpose is to make the type checker happy,
    // since now we can return the struct as a value instead of not returning
    // anything (isn't it beautiful?)
    struct { reg_t eax; reg_t edx; } ret;

    // set stack pointer and call library function
    asm (
        "movl %%esp, %%ebx\n\t" // use ebx to hold esp until the function returns
        "movl %0, %%esp\n\t"
        "calll *%1\n\t"
        :: "g"(esp), "r"(targetpc), "c"(ecx), "d"(edx)
        : "ebx", "esp"
    );

    // put fpstt in register to avoid double memory load
    const register unsigned int tmp_fpstt = fpstt;

    // copy return value fo library function to virtual environment, return
    // edx/eax by value so that we don't use pointers to virtual registers
    asm (
        "fstpt %2\n\t"
        "movl %%ebx, %%esp"     // restore saved stack pointer
        : "=a"(ret.eax), "=d"(ret.edx), "=m"(fpregs[tmp_fpstt].d)
        :: "eax", "edx", "esp"
    );

    // activate ST0
    // FIXME: is this even necessary?
    // TODO: test SPEC2006 with this disabled, if that works remove it
    fptags[tmp_fpstt] = 0;

    // this is optimized away, as explained above
    return ((uint64_t)ret.edx << 32) | ret.eax;
}

void __attribute__((always_inline)) helper_extern_stub() {
    // return address should be on top of virtual stack, pop it
    addr_t retaddr = __ldl_mmu(R_ESP, 0);
    R_ESP += sizeof (stackword_t);

    // PC should contain the address of the target function, call it
    uint64_t ret = helper_stub_trampoline(R_EDX, R_ECX, R_ESP, PC);

    // copy return value fo library function to virtual environment
    R_EAX = ret & 0xffffffff;
    R_EDX = ret >> 32;

    // jump to the return address
    PC = retaddr;
}

// FIXME: pass float in argument
void __attribute__((always_inline)) virtualize_return_float() {
    asm ("fstpt %0" : "=m"(fpregs[fpstt].d) ::);
    fptags[fpstt] = 0;
}

void __attribute__((always_inline)) virtualize_return_i32(uint32_t ret) {
    R_EAX = ret;
}

void __attribute__((always_inline)) virtualize_return_i64(uint64_t ret) {
    R_EAX = ret & 0xffffffff;
    R_EDX = ret >> 32;
}

void __attribute__((noinline)) helper_debug_state() {
#define WRITE(type, label, var) { OUTPUT(label "="); pad_right(DEBUG_FD, mywrite_##type(DEBUG_FD, var), ' ', 8); }
    WRITE(hex, "pc", PC);
    WRITE(hex, " eax", R_EAX);
    WRITE(hex, " ebx", R_EBX);
    WRITE(hex, " ecx", R_ECX);
    WRITE(hex, " edx", R_EDX);
    WRITE(hex, " ebp", R_EBP);
    WRITE(hex, " esp", R_ESP);
    WRITE(hex, " esi", R_ESI);
    WRITE(hex, " edi", R_EDI);
    WRITE(dec, " cc_src", cc_src);
    WRITE(dec, " cc_dst", cc_dst);
    WRITE(dec, " cc_op", cc_op);
    WRITE(dec, " df", df);
    WRITE(hex, " mflags", mflags);
    mywrite_char(DEBUG_FD, '\n');
    WRITE(hex, "[STACK]", stack[STACK_SIZE - 1]);
    WRITE(hex, "", stack[STACK_SIZE - 2]);
    WRITE(hex, "", stack[STACK_SIZE - 3]);
    WRITE(hex, "", stack[STACK_SIZE - 4]);
    WRITE(hex, "", stack[STACK_SIZE - 5]);
    WRITE(hex, "", stack[STACK_SIZE - 6]);
    WRITE(hex, "", stack[STACK_SIZE - 7]);
    WRITE(hex, "", stack[STACK_SIZE - 8]);
    mywrite_char(DEBUG_FD, '\n');
    WRITE(hex, "       ", stack[STACK_SIZE - 9]);
    WRITE(hex, "", stack[STACK_SIZE - 10]);
    WRITE(hex, "", stack[STACK_SIZE - 11]);
    WRITE(hex, "", stack[STACK_SIZE - 12]);
    WRITE(hex, "", stack[STACK_SIZE - 13]);
    WRITE(hex, "", stack[STACK_SIZE - 14]);
    WRITE(hex, "", stack[STACK_SIZE - 15]);
    WRITE(hex, "", stack[STACK_SIZE - 16]);
    mywrite_char(DEBUG_FD, '\n');
    WRITE(hex, "       ", stack[STACK_SIZE - 17]);
    WRITE(hex, "", stack[STACK_SIZE - 18]);
    WRITE(hex, "", stack[STACK_SIZE - 19]);
    WRITE(hex, "", stack[STACK_SIZE - 20]);
    WRITE(hex, "", stack[STACK_SIZE - 21]);
    WRITE(hex, "", stack[STACK_SIZE - 22]);
    WRITE(hex, "", stack[STACK_SIZE - 23]);
    WRITE(hex, "", stack[STACK_SIZE - 24]);
    mywrite_char(DEBUG_FD, '\n');
    WRITE(hex, "*R_ESP and Up", *(stackword_t*)R_ESP);
    WRITE(hex, "", *(stackword_t*)(R_ESP + 1));
    WRITE(hex, "", *(stackword_t*)(R_ESP + 2));
    WRITE(hex, "", *(stackword_t*)(R_ESP + 3));
    WRITE(hex, "", *(stackword_t*)(R_ESP + 4));
    WRITE(hex, "", *(stackword_t*)(R_ESP + 5));
    WRITE(hex, "", *(stackword_t*)(R_ESP + 6));
    WRITE(hex, "", *(stackword_t*)(R_ESP + 7));
    mywrite_char(DEBUG_FD, '\n');
    WRITE(hex, "[FPSTT]", fpstt);
    mywrite_char(DEBUG_FD, '\n');
    WRITE(hex, "[FPTAGS]", fptags[7]);
    WRITE(hex, "", fptags[6]);
    WRITE(hex, "", fptags[5]);
    WRITE(hex, "", fptags[4]);
    WRITE(hex, "", fptags[3]);
    WRITE(hex, "", fptags[2]);
    WRITE(hex, "", fptags[1]);
    WRITE(hex, "", fptags[0]);
    mywrite_char(DEBUG_FD, '\n');
    WRITE(hex, "[FPREGS]", fpregs[7].d.low & 0xffffffff);
    WRITE(hex, "", fpregs[7].d.low >> 32);
    WRITE(hex, "", fpregs[6].d.low & 0xffffffff);
    WRITE(hex, "", fpregs[6].d.low >> 32);
    WRITE(hex, "", fpregs[5].d.low & 0xffffffff);
    WRITE(hex, "", fpregs[5].d.low >> 32);
    WRITE(hex, "", fpregs[4].d.low & 0xffffffff);
    WRITE(hex, "", fpregs[4].d.low >> 32);
    mywrite_char(DEBUG_FD, '\n');
    WRITE(hex, "        ", fpregs[3].d.low & 0xffffffff);
    WRITE(hex, "", fpregs[3].d.low >> 32);
    WRITE(hex, "", fpregs[2].d.low & 0xffffffff);
    WRITE(hex, "", fpregs[2].d.low >> 32);
    WRITE(hex, "", fpregs[1].d.low & 0xffffffff);
    WRITE(hex, "", fpregs[1].d.low >> 32);
    WRITE(hex, "", fpregs[0].d.low & 0xffffffff);
    WRITE(hex, "", fpregs[0].d.low >> 32);
    mywrite_char(DEBUG_FD, '\n');
    WRITE(hex, "[FPSTATUS]", fp_status.float_detect_tininess);
    WRITE(hex, "", fp_status.float_rounding_mode);
    WRITE(hex, "", fp_status.float_exception_flags);
    WRITE(hex, "", fp_status.floatx80_rounding_precision);
    WRITE(hex, "", fp_status.flush_to_zero);
    WRITE(hex, "", fp_status.flush_inputs_to_zero);
    WRITE(hex, "", fp_status.default_nan_mode);
    mywrite_uint64(DEBUG_FD, 1, 16);
    mywrite_char(DEBUG_FD, '\n');
    OUTPUT("---------------------------------------------------------------------------------\n");
#undef WRITE
}

void helper_break() {
    asm ("int $0x3");
}

// redefine the following functions to avoid cases that handle symbolic memory
// in the default implementations

uint64_t __attribute__((always_inline)) ldq_data(uint32_t ptr) {
    return __ldq_mmu(ptr, 0);
}

uint32_t __attribute__((always_inline)) lduw_data(uint32_t ptr) {
    return __ldw_mmu(ptr, 0);
}

void __attribute__((always_inline)) stq_data(uint32_t ptr, uint64_t value) {
    __stq_mmu(ptr, value, 0);
}

void __attribute__((always_inline)) stw_data(uint32_t ptr, uint32_t value) {
    __stw_mmu(ptr, value, 0);
}

void helper_s2e_tcg_custom_instruction_handler(uint32_t opcode) {
    OUTPUT("custom s2e instruction with opcode ");
    mywrite_hex(DEBUG_FD, opcode);
    OUTPUT(" called");
}
