#ifndef LOGFILE_H
#define LOGFILE_H

#define LOG_SEPARATOR 0
#define STATE_FORK    1
#define STATE_SWITCH  2

static inline void logfile_write_addr(FILE *f, uint32_t addr)
{
    fwrite(&addr, sizeof(uint32_t), 1, f);
    fflush(f);
}

#endif  // LOGFILE_H
