#include "RegisterPasses.h"
#include "AddCustomHelperVars.h"
#include "Lifting/AddMemArray.h"
#include "Lifting/InlineLibCallArgs.h"
#include "Lifting/RemoveS2EHelpers.h"
#include "Lowering/RemoveSections.h"
#include "SetDataLayout32.h"
#include "TagInstPc.h"
#include <array>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <map>

using namespace binrec;
using namespace llvm;

namespace {
constexpr std::array<std::pair<StringRef, void (*)(ModulePassManager &)>, 4> module_passes = {{
    {"add-custom-helper-vars", [](auto &mpm) { mpm.addPass(AddCustomHelperVarsPass{}); }},
    {"remove-qemu-helpers", [](auto &mpm) { mpm.addPass(RemoveS2EHelpersPass{}); }},
    {"set-data-layout-32", [](auto &mpm) { mpm.addPass(SetDataLayout32Pass{}); }},
    {"tag-inst-pc", [](auto &mpm) { mpm.addPass(TagInstPcPass{}); }},
}};

constexpr std::array<std::pair<StringRef, void (*)(ModulePassManager &)>, 2> debug_module_passes = {{
    {"dot-merged-blocks", addDotMergedBlocksPass},
    {"dot-overlaps", addDotOverlapsPass},
}};

constexpr std::array<std::pair<StringRef, void (*)(ModulePassManager &)>, 28> lifting_module_passes = {{
    {"add-debug-output", addAddDebugPass},
    {"add-mem-array", [](auto &mpm) { mpm.addPass(AddMemArrayPass{}); }},
    {"elf-symbols", addELFSymbolsPass},
    {"extern-plt", addExternPLTPass},
    {"fix-cfg", addFixCFGPass},
    {"fix-overlaps", addFixOverlapsPass},
    {"implement-lib-call-stubs", addImplementLibCallStubsPass},
    {"implement-lib-calls-new-plt", addImplementLibCallsNewPLTPass},
    {"inline-lib-call-args", [](auto &mpm) { mpm.addPass(InlineLibCallArgsPass{}); }},
    {"inline-stubs", addInlineStubsPass},
    {"insert-calls", addInsertCallsPass},
    {"insert-tramp-for-rec-funcs", addInsertTrampForRecFuncsPass},
    {"internalize-functions", addInternalizeFunctionsPass},
    {"internalize-globals", addInternalizeGlobalsPass},
    {"lib-call-new-plt", addLibCallNewPLTPass},
    {"merge-functions", addMergeFunctionsPass},
    {"pc-jumps", addPcJumpsPass},
    {"prune-libargs-push", addPruneLibargsPushPass},
    {"prune-null-succs", addPruneNullSuccsPass},
    {"prune-retaddr-push", addPruneRetaddrPushPass},
    {"prune-trivially-dead-succs", addPruneTriviallyDeadSuccsPass},
    {"recov-func-trampolines", addRecovFuncTrampolinesPass},
    {"recover-functions", addRecoverFunctionsPass},
    {"remove-metadata", addRemoveMetadataPass},
    {"replace-dynamic-symbols", addReplaceDynamicSymbolsPass},
    {"successor-lists", addSuccessorListsPass},
    {"unalign-stack", addUnalignStackPass},
}};

constexpr std::array<std::pair<StringRef, void (*)(ModulePassManager &)>, 4> lowering_module_passes = {{
    {"halt-on-declarations", addHaltOnDeclarationsPass},
    {"internalize-debug-helpers", addInternalizeDebugHelpersPass},
    {"remove-sections", [](ModulePassManager &mpm) { mpm.addPass(RemoveSectionsPass{}); }},
}};

constexpr std::array<std::pair<StringRef, void (*)(ModulePassManager &)>, 7> merging_module_passes = {{
    {"decompose-env", addDecomposeEnvPass},
    {"externalize-functions", addExternalizeFunctionsPass},
    {"internalize-imports", addInternalizeImportsPass},
    {"rename-block-funcs", addRenameBlockFuncsPass},
    {"rename-env", addRenameEnvPass},
    {"unflatten-env", addUnflattenEnvPass},
    {"unimplement-custom-helpers", addUnimplementCustomHelpersPass},
}};

void registerBinRecPasses(PassBuilder &pb) {
    std::map<StringRef, void (*)(ModulePassManager &)> module_pass_map;
    std::copy(module_passes.begin(), module_passes.end(), std::inserter(module_pass_map, module_pass_map.begin()));
    std::copy(debug_module_passes.begin(), debug_module_passes.end(),
              std::inserter(module_pass_map, module_pass_map.begin()));
    std::copy(lifting_module_passes.begin(), lifting_module_passes.end(),
              std::inserter(module_pass_map, module_pass_map.begin()));
    std::copy(lowering_module_passes.begin(), lowering_module_passes.end(),
              std::inserter(module_pass_map, module_pass_map.begin()));
    std::copy(merging_module_passes.begin(), merging_module_passes.end(),
              std::inserter(module_pass_map, module_pass_map.begin()));

    pb.registerPipelineParsingCallback(
        [module_pass_map{std::move(module_pass_map)}](StringRef name, ModulePassManager &mpm,
                                                      ArrayRef<PassBuilder::PipelineElement>) {
            auto module_pass_it = module_pass_map.find(name);
            if (module_pass_it != module_pass_map.end()) {
                module_pass_it->second(mpm);
                return true;
            }

            return false;
        });
}
} // namespace

auto binrec::getBinRecPluginInfo() -> llvm::PassPluginLibraryInfo {
    return {LLVM_PLUGIN_API_VERSION, "BinRec", LLVM_VERSION_STRING, registerBinRecPasses};
}
