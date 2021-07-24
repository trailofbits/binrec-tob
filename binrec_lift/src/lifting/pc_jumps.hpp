#ifndef BINREC_PC_JUMPS_HPP
#define BINREC_PC_JUMPS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Replace stores to PC followed by returns with branches
    class PcJumpsPass : public llvm::PassInfoMixin<PcJumpsPass> {
    public:
        /// For each BB_xxxxxx in @wrapper, replace:
        ///   ret void !{blockaddress(@wrapper, %BB_xxxxxx), ...}
        /// with:
        ///   %pc = load i32* @PC
        ///   switch %pc, label %error [
        ///       i32 xxxxxx, label %BB_xxxxxx
        ///       ...
        ///   ]
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
