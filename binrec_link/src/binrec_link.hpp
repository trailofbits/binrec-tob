#ifndef BINREC_LINK_HPP
#define BINREC_LINK_HPP

#include "link_context.hpp"
//#include <llvm/Support/Error.h>

namespace binrec {
    auto run_link(LinkContext &ctx) -> llvm::Error;
    void cleanup_link(LinkContext &ctx);
} // namespace binrec

#endif
