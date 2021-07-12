#ifndef BINREC_LINKCONTEXT_H
#define BINREC_LINKCONTEXT_H

#include <llvm/Object/Binary.h>

namespace binrec {
class LinkContext {
public:
    llvm::SmallString<128> workDir;
    llvm::object::OwningBinary<llvm::object::Binary> originalBinary;
    llvm::object::OwningBinary<llvm::object::Binary> recoveredBinary;

    LinkContext() = default;
    LinkContext(const LinkContext &) = delete;
    LinkContext(LinkContext &&) = delete;
    auto operator=(const LinkContext &) -> LinkContext & = delete;
    auto operator=(LinkContext &&) -> LinkContext & = delete;
};
} // namespace binrec

#endif
