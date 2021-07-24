#ifndef BINREC_ELF_EXE_TO_OBJ_HPP
#define BINREC_ELF_EXE_TO_OBJ_HPP

#include "link_context.hpp"
#include "section_info.hpp"

namespace binrec {
    auto elf_exe_to_obj(llvm::StringRef output_path, LinkContext &ctx)
        -> llvm::ErrorOr<std::vector<SectionInfo>>;
} // namespace binrec

#endif
