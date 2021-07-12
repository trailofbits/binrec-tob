#ifndef BINREC_REGISTERPASSES_H
#define BINREC_REGISTERPASSES_H

#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassPlugin.h>

namespace binrec {
// Debug/Module passes
void addDotMergedBlocksPass(llvm::ModulePassManager &mpm);
void addDotOverlapsPass(llvm::ModulePassManager &mpm);

// Lifiting passes
void addAddDebugPass(llvm::ModulePassManager &mpm);
void addELFSymbolsPass(llvm::ModulePassManager &mpm);
void addExternPLTPass(llvm::ModulePassManager &mpm);
void addFixCFGPass(llvm::ModulePassManager &mpm);
void addFixOverlapsPass(llvm::ModulePassManager &mpm);
void addImplementLibCallsNewPLTPass(llvm::ModulePassManager &mpm);
void addImplementLibCallStubsPass(llvm::ModulePassManager &mpm);
void addInlineQemuOpHelpersPass(llvm::ModulePassManager &mpm);
void addInlineStubsPass(llvm::ModulePassManager &mpm);
void addInsertCallsPass(llvm::ModulePassManager &mpm);
void addInsertTrampForRecFuncsPass(llvm::ModulePassManager &mpm);
void addInternalizeFunctionsPass(llvm::ModulePassManager &mpm);
void addInternalizeGlobalsPass(llvm::ModulePassManager &mpm);
void addLibCallNewPLTPass(llvm::ModulePassManager &mpm);
void addMergeFunctionsPass(llvm::ModulePassManager &mpm);
void addPcJumpsPass(llvm::ModulePassManager &mpm);
void addPruneLibargsPushPass(llvm::ModulePassManager &mpm);
void addPruneNullSuccsPass(llvm::ModulePassManager &mpm);
void addPruneRetaddrPushPass(llvm::ModulePassManager &mpm);
void addPruneTriviallyDeadSuccsPass(llvm::ModulePassManager &mpm);
void addRecoverFunctionsPass(llvm::ModulePassManager &mpm);
void addRecovFuncTrampolinesPass(llvm::ModulePassManager &mpm);
void addRemoveMetadataPass(llvm::ModulePassManager &mpm);
void addReplaceDynamicSymbolsPass(llvm::ModulePassManager &mpm);
void addSuccessorListsPass(llvm::ModulePassManager &mpm);
void addUnalignStackPass(llvm::ModulePassManager &mpm);

// Lowering/Module passes
void addHaltOnDeclarationsPass(llvm::ModulePassManager &mpm);
void addInternalizeDebugHelpersPass(llvm::ModulePassManager &mpm);

// Merging/Module passes
void addDecomposeEnvPass(llvm::ModulePassManager &mpm);
void addExternalizeFunctionsPass(llvm::ModulePassManager &mpm);
void addInternalizeImportsPass(llvm::ModulePassManager &mpm);
void addRenameBlockFuncsPass(llvm::ModulePassManager &mpm);
void addRenameEnvPass(llvm::ModulePassManager &mpm);
void addUnflattenEnvPass(llvm::ModulePassManager &mpm);
void addUnimplementCustomHelpersPass(llvm::ModulePassManager &mpm);

auto getBinRecPluginInfo() -> llvm::PassPluginLibraryInfo;
} // namespace binrec

#endif
