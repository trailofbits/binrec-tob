#ifndef BINREC_SECTIONLINK_H
#define BINREC_SECTIONLINK_H

#include "LinkContext.h"
#include "SectionInfo.h"

namespace binrec {
auto linkRecoveredBinary(const std::vector<SectionInfo> &sections, llvm::StringRef ldScriptPath,
                         llvm::StringRef outputPath, const std::vector<llvm::StringRef> inputPaths, LinkContext &ctx)
    -> llvm::Error;
}

#endif
