#ifndef BINREC_SECTION_INFO_HPP
#define BINREC_SECTION_INFO_HPP

#include <llvm/ADT/SmallString.h>

namespace binrec {
    struct SectionInfo {
        llvm::SmallString<128> name;
        uint64_t virtual_address = 0;
        uint64_t flags = 0;
        uint64_t size = 0;
        bool nobits = false;
    };
} // namespace binrec

#endif
