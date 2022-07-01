#ifndef BINREC_SECTION_LINK_HPP
#define BINREC_SECTION_LINK_HPP

#include "link_context.hpp"
#include "section_info.hpp"

namespace binrec {
    auto link_recovered_binary(
        const std::vector<SectionInfo> &sections,
        std::string ld_script_path,
        std::string output_path,
        const std::vector<std::string> &input_paths,
        LinkContext &ctx) -> llvm::Error;
} // namespace binrec

#endif
