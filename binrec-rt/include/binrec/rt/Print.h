#ifndef BINREC_PRINT_H
#define BINREC_PRINT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>

int binrec_rt_printf(const char *format, ...);
int binrec_rt_fdprintf(int fd, const char *format, ...);
int binrec_rt_sprintf(char *buffer, size_t size, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
