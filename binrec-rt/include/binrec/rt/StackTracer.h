#ifndef BINREC_STACKTRACER_H
#define BINREC_STACKTRACER_H

#include "Syscall.h"

#ifdef __cplusplus
extern "C" {
#endif
void binrec_rt_stacksize_save();

void binrec_rt_frame_begin(intptr_t pc, intptr_t sp);
void binrec_rt_frame_end(intptr_t pc);
void binrec_rt_frame_update(intptr_t sp);
void binrec_rt_frame_save();
#ifdef __cplusplus
}
#endif

#endif