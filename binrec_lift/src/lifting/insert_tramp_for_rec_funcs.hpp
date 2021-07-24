#ifndef BINREC_INSERT_TRAMP_FOR_REC_FUNCS_HPP
#define BINREC_INSERT_TRAMP_FOR_REC_FUNCS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// insert trampolines for orig binary to call recovered functions
    ///
    /// XXX: This pass currently assumes that all the callback functions have
    /// returned from the same location.
    /// The reason for this assumption is that, the metadata we collect from the
    /// frontend is not enough to find all the return BBs inside a callback function.
    /// We can actually find other return BBs with an assumption of callback function
    /// never jumps to its entry BB. This assumption wouldn't hold for some of the
    /// callback functions such as tail recursive functions. If the above assumption
    /// holds, this is how we can find other return BBs in the callback functions:
    /// Follow the succs starting from entry BB of callback function.
    /// At every BB, check if any of the succs is entry BB. If it is, then the
    /// current BB is return BB. Continue until we reach return BB that is in
    /// entryToReturn.(key is address of lib function in plt. For example: qsort
    /// takes compare function and return BB of compare function can be found using
    /// address of qsort in plt)
    class InsertTrampForRecFuncsPass : public llvm::PassInfoMixin<InsertTrampForRecFuncsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
