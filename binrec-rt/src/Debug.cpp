#include "binrec/rt/CpuX86.h"
#include "binrec/rt/Print.h"

void __attribute__((noinline)) helper_debug_state() {
    binrec_rt_printf("pc=%x, ", PC);
    binrec_rt_printf("eax=%x, ", R_EAX);
    binrec_rt_printf("ebx=%x, ", R_EBX);
    binrec_rt_printf("ecx=%x, ", R_ECX);
    binrec_rt_printf("edx=%x, ", R_EDX);
    binrec_rt_printf("ebp=%x, ", R_EBP);
    binrec_rt_printf("esp=%x, ", R_ESP);
    binrec_rt_printf("esi=%x, ", R_ESI);
    binrec_rt_printf("edi=%x, ", R_EDI);
    binrec_rt_printf("cc_src=%d, ", cc_src);
    binrec_rt_printf("cc_dst=%d, ", cc_dst);
    binrec_rt_printf("cc_op=%d, ", cc_op);
    binrec_rt_printf("df=%d, ", df);
    binrec_rt_printf("mflags=%x\n", mflags);
    for (int i = 0; i < 3; ++i) {
        uint32_t *stackptr = stack + STACK_SIZE - 8 * i;
        binrec_rt_printf("[STACK]=%x %x %x %x %x %x %x %x\n", stackptr - 1, stackptr - 2, stackptr - 3, stackptr - 4,
                         stackptr - 5, stackptr - 6, stackptr - 7, stackptr - 8);
    }
    binrec_rt_printf("*R_ESP and Up=%x, ", *(stackword_t *)R_ESP);
    binrec_rt_printf("=%x, ", *(stackword_t *)(R_ESP + 1));
    binrec_rt_printf("=%x, ", *(stackword_t *)(R_ESP + 2));
    binrec_rt_printf("=%x, ", *(stackword_t *)(R_ESP + 3));
    binrec_rt_printf("=%x, ", *(stackword_t *)(R_ESP + 4));
    binrec_rt_printf("=%x, ", *(stackword_t *)(R_ESP + 5));
    binrec_rt_printf("=%x, ", *(stackword_t *)(R_ESP + 6));
    binrec_rt_printf("=%x, \n", *(stackword_t *)(R_ESP + 7));
    binrec_rt_printf("[FPSTT]=%x, \n", fpstt);
    binrec_rt_printf("[FPTAGS]=%x, ", fptags[7]);
    binrec_rt_printf("=%x, ", fptags[6]);
    binrec_rt_printf("=%x, ", fptags[5]);
    binrec_rt_printf("=%x, ", fptags[4]);
    binrec_rt_printf("=%x, ", fptags[3]);
    binrec_rt_printf("=%x, ", fptags[2]);
    binrec_rt_printf("=%x, ", fptags[1]);
    binrec_rt_printf("=%x, \n", fptags[0]);
    binrec_rt_printf("[FPREGS]=%x, ", fpregs[7].d.low & 0xffffffff);
    binrec_rt_printf("=%x, ", fpregs[7].d.low >> 32);
    binrec_rt_printf("=%x, ", fpregs[6].d.low & 0xffffffff);
    binrec_rt_printf("=%x, ", fpregs[6].d.low >> 32);
    binrec_rt_printf("=%x, ", fpregs[5].d.low & 0xffffffff);
    binrec_rt_printf("=%x, ", fpregs[5].d.low >> 32);
    binrec_rt_printf("=%x, ", fpregs[4].d.low & 0xffffffff);
    binrec_rt_printf("=%x, \n", fpregs[4].d.low >> 32);
    binrec_rt_printf("        =%x, ", fpregs[3].d.low & 0xffffffff);
    binrec_rt_printf("=%x, ", fpregs[3].d.low >> 32);
    binrec_rt_printf("=%x, ", fpregs[2].d.low & 0xffffffff);
    binrec_rt_printf("=%x, ", fpregs[2].d.low >> 32);
    binrec_rt_printf("=%x, ", fpregs[1].d.low & 0xffffffff);
    binrec_rt_printf("=%x, ", fpregs[1].d.low >> 32);
    binrec_rt_printf("=%x, ", fpregs[0].d.low & 0xffffffff);
    binrec_rt_printf("=%x, \n", fpregs[0].d.low >> 32);
    binrec_rt_printf("[FPSTATUS]=%x, ", fp_status.float_detect_tininess);
    binrec_rt_printf("=%x, ", fp_status.float_rounding_mode);
    binrec_rt_printf("=%x, ", fp_status.float_exception_flags);
    binrec_rt_printf("=%x, ", fp_status.floatx80_rounding_precision);
    binrec_rt_printf("=%x, ", fp_status.flush_to_zero);
    binrec_rt_printf("=%x, ", fp_status.flush_inputs_to_zero);
    binrec_rt_printf("=%x, \n", fp_status.default_nan_mode);
    binrec_rt_printf("---------------------------------------------------------------------------------\n");
#undef WRITE
}