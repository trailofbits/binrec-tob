#ifndef BINREC_SECTIONINFO_H
#define BINREC_SECTIONINFO_H

#include <llvm/ADT/SmallString.h>

namespace binrec {
struct SectionInfo {
    llvm::SmallString<128> name;
    uint64_t virtualAddress = 0;
    uint64_t flags = 0;
    uint64_t size = 0;
    bool nobits = false;
};
} // namespace binrec

#endif
