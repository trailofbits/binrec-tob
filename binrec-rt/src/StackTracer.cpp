#include "binrec/rt/StackTracer.h"
#include "binrec/rt/Print.h"
#include "binrec/rt/Syscall.h"
#include <algorithm>
#include <cstdint>
#include <fcntl.h>

namespace {
struct StackSizeMapEntry {
    const char *fnName;
    std::int32_t *maxStackSize;
    std::int32_t *stackDifference;
};
} // namespace

extern "C" {
std::uint32_t binrec_stacksize_count __attribute__((weak)); // NOLINT
StackSizeMapEntry *binrec_stacksize __attribute__((weak));  // NOLINT
}

void binrec_rt_stacksize_save() {
    if (binrec_stacksize != nullptr) {
        int fd = binrec_rt_open("stackSize.json", O_WRONLY | O_CREAT | O_TRUNC);
        binrec_rt_fdprintf(fd, "{\n");

        binrec_rt_fdprintf(fd, "\t\"stackSizes\": {");
        const char *sep = "\n\t\t";
        std::for_each(binrec_stacksize, binrec_stacksize + binrec_stacksize_count, [fd, &sep](StackSizeMapEntry entry) {
            binrec_rt_fdprintf(fd, "%s\"%s\": %u", sep, entry.fnName, *entry.maxStackSize);
            sep = ",\n\t\t";
        });
        binrec_rt_fdprintf(fd, "\n\t},\n");

        binrec_rt_fdprintf(fd, "\t\"stackDifference\": {");
        sep = "\n\t\t";
        std::for_each(binrec_stacksize, binrec_stacksize + binrec_stacksize_count, [fd, &sep](StackSizeMapEntry entry) {
            binrec_rt_fdprintf(fd, "%s\"%s\": %u", sep, entry.fnName, *entry.stackDifference);
            sep = ",\n\t\t";
        });
        binrec_rt_fdprintf(fd, "\n\t}\n");

        binrec_rt_fdprintf(fd, "}\n");
        binrec_rt_close(fd);
    }
}

void binrec_rt_frame_begin(intptr_t pc, intptr_t rbp) {
    const char buf[] = "binrec_rt_frame_begin\n";
    binrec_rt_write(STDOUT_FILENO, buf, sizeof(buf));
}

void binrec_rt_frame_end(intptr_t pc) {}

void binrec_rt_frame_update(intptr_t rsp) {
    const char buf[] = "binrec_rt_frame_update\n";
    binrec_rt_write(STDOUT_FILENO, buf, sizeof(buf));
}

void binrec_rt_frame_save() {}