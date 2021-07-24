#include "binrec/rt/cpu_x86.hpp"
#include <cstdio>

void helper_debug_state()
{
    fprintf(stderr, "pc=%x, ", PC);
    fprintf(stderr, "eax=%x, ", R_EAX);
    fprintf(stderr, "ebx=%x, ", R_EBX);
    fprintf(stderr, "ecx=%x, ", R_ECX);
    fprintf(stderr, "edx=%x, ", R_EDX);
    fprintf(stderr, "ebp=%x, ", R_EBP);
    fprintf(stderr, "esp=%x, ", R_ESP);
    fprintf(stderr, "esi=%x, ", R_ESI);
    fprintf(stderr, "edi=%x, ", R_EDI);
    fprintf(stderr, "cc_src=%d, ", cc_src);
    fprintf(stderr, "cc_dst=%d, ", cc_dst);
    fprintf(stderr, "cc_op=%d, ", cc_op);
    fprintf(stderr, "df=%d, ", df);
    fprintf(stderr, "mflags=%x\n", mflags);
    for (int i = 0; i < 3; ++i) {
        uint32_t *stackptr = stack + STACK_SIZE - 8 * i;
        fprintf(
            stderr,
            "[STACK]=%x %x %x %x %x %x %x %x\n",
            stackptr[-1],
            stackptr[-2],
            stackptr[-3],
            stackptr[-4],
            stackptr[-5],
            stackptr[-6],
            stackptr[-7],
            stackptr[-8]);
    }
    fprintf(stderr, "*R_ESP and Up=%x, ", *(stackword_t *)R_ESP);
    fprintf(stderr, "=%x, ", *(stackword_t *)(R_ESP + 1));
    fprintf(stderr, "=%x, ", *(stackword_t *)(R_ESP + 2));
    fprintf(stderr, "=%x, ", *(stackword_t *)(R_ESP + 3));
    fprintf(stderr, "=%x, ", *(stackword_t *)(R_ESP + 4));
    fprintf(stderr, "=%x, ", *(stackword_t *)(R_ESP + 5));
    fprintf(stderr, "=%x, ", *(stackword_t *)(R_ESP + 6));
    fprintf(stderr, "=%x, \n", *(stackword_t *)(R_ESP + 7));
    fprintf(stderr, "[FPSTT]=%x, \n", fpstt);
    fprintf(stderr, "[FPTAGS]=%x, ", fptags[7]);
    fprintf(stderr, "=%x, ", fptags[6]);
    fprintf(stderr, "=%x, ", fptags[5]);
    fprintf(stderr, "=%x, ", fptags[4]);
    fprintf(stderr, "=%x, ", fptags[3]);
    fprintf(stderr, "=%x, ", fptags[2]);
    fprintf(stderr, "=%x, ", fptags[1]);
    fprintf(stderr, "=%x, \n", fptags[0]);
    fprintf(stderr, "[FPREGS]=%llx, ", fpregs[7].d.low & 0xffffffff);
    fprintf(stderr, "=%llx, ", fpregs[7].d.low >> 32);
    fprintf(stderr, "=%llx, ", fpregs[6].d.low & 0xffffffff);
    fprintf(stderr, "=%llx, ", fpregs[6].d.low >> 32);
    fprintf(stderr, "=%llx, ", fpregs[5].d.low & 0xffffffff);
    fprintf(stderr, "=%llx, ", fpregs[5].d.low >> 32);
    fprintf(stderr, "=%llx, ", fpregs[4].d.low & 0xffffffff);
    fprintf(stderr, "=%llx, \n", fpregs[4].d.low >> 32);
    fprintf(stderr, "        =%llx, ", fpregs[3].d.low & 0xffffffff);
    fprintf(stderr, "=%llx, ", fpregs[3].d.low >> 32);
    fprintf(stderr, "=%llx, ", fpregs[2].d.low & 0xffffffff);
    fprintf(stderr, "=%llx, ", fpregs[2].d.low >> 32);
    fprintf(stderr, "=%llx, ", fpregs[1].d.low & 0xffffffff);
    fprintf(stderr, "=%llx, ", fpregs[1].d.low >> 32);
    fprintf(stderr, "=%llx, ", fpregs[0].d.low & 0xffffffff);
    fprintf(stderr, "=%llx, \n", fpregs[0].d.low >> 32);
    fprintf(stderr, "[FPSTATUS]=%x, ", fp_status.float_detect_tininess);
    fprintf(stderr, "=%x, ", fp_status.float_rounding_mode);
    fprintf(stderr, "=%x, ", fp_status.float_exception_flags);
    fprintf(stderr, "=%x, ", fp_status.floatx80_rounding_precision);
    fprintf(stderr, "=%x, ", fp_status.flush_to_zero);
    fprintf(stderr, "=%x, ", fp_status.flush_inputs_to_zero);
    fprintf(stderr, "=%x, \n", fp_status.default_nan_mode);
    fprintf(
        stderr,
        "---------------------------------------------------------------------------------\n");
#undef WRITE
}