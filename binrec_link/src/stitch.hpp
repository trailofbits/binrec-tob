#ifndef BINREC_STITCH_HPP
#define BINREC_STITCH_HPP

#include "link_context.hpp"

namespace binrec {
    auto stitch(llvm::StringRef binary_path, LinkContext &ctx) -> std::error_code;
} // namespace binrec

#endif
