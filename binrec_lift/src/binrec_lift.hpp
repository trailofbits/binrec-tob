#ifndef BINREC_LIFT_HPP
#define BINREC_LIFT_HPP

#include <llvm/Passes/PassBuilder.h>

#include "lift_context.hpp"

namespace binrec {
    auto build_pipeline(LiftContext &ctx, llvm::PassBuilder &pb) -> llvm::ModulePassManager;
    auto initialize_lift(LiftContext &ctx, llvm::ModulePassManager &mpm) -> std::error_code;
    void initialize_pipeline(LiftContext &ctx, llvm::PassBuilder& pb, llvm::ModuleAnalysisManager &mam, llvm::AAManager &aa);
    void run_lift(LiftContext& ctx);
}

#endif
