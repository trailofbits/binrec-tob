#ifndef BINREC_LINK_CONTEXT_HPP
#define BINREC_LINK_CONTEXT_HPP

#include <llvm/Object/Binary.h>

namespace binrec {
    class LinkContext {
    public:
        llvm::SmallString<128> work_dir;
        llvm::object::OwningBinary<llvm::object::Binary> original_binary;
        llvm::object::OwningBinary<llvm::object::Binary> recovered_binary;
        std::string recovered_filename;
        std::string librt_filename;
        std::string ld_script_filename;
        std::string output_filename;
        std::string dependencies_filename;
        bool harden;

        LinkContext() = default;
        LinkContext(const LinkContext &) = delete;
        LinkContext(LinkContext &&) = delete;
        auto operator=(const LinkContext &) -> LinkContext & = delete;
        auto operator=(LinkContext &&) -> LinkContext & = delete;
    };
} // namespace binrec

#endif
