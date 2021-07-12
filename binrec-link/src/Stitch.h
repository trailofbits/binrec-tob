#ifndef BINREC_STITCH_H
#define BINREC_STITCH_H

#include "LinkContext.h"

namespace binrec {
auto stitch(llvm::StringRef binaryPath, LinkContext &ctx) -> std::error_code;
}

#endif
