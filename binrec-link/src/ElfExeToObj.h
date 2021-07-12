#ifndef BINREC_ELFEXETOOBJ_H
#define BINREC_ELFEXETOOBJ_H

#include "LinkContext.h"
#include "SectionInfo.h"

namespace binrec {
auto elfExeToObj(llvm::StringRef outputPath, LinkContext &ctx) -> llvm::ErrorOr<std::vector<SectionInfo>>;
} // namespace binrec

#endif
