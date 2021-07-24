#include "add_custom_helper_vars.hpp"
#include "analysis/env_alias_analysis.hpp"
#include "analysis/trace_info_analysis.hpp"
#include "debug/call_tracer.hpp"
#include "inline_wrapper.hpp"
#include "ir/selectors.hpp"
#include "lifting/add_debug.hpp"
#include "lifting/add_mem_array.hpp"
#include "lifting/elf_symbols.hpp"
#include "lifting/extern_plt.hpp"
#include "lifting/fix_cfg.hpp"
#include "lifting/fix_overlaps.hpp"
#include "lifting/global_env_to_alloca.hpp"
#include "lifting/implement_lib_call_stubs.hpp"
#include "lifting/implement_lib_calls_new_plt.hpp"
#include "lifting/inline_lib_call_args.hpp"
#include "lifting/inline_qemu_op_helpers.hpp"
#include "lifting/inline_stubs.hpp"
#include "lifting/insert_calls.hpp"
#include "lifting/insert_tramp_for_rec_funcs.hpp"
#include "lifting/internalize_functions.hpp"
#include "lifting/internalize_globals.hpp"
#include "lifting/lib_call_new_plt.hpp"
#include "lifting/pc_jumps.hpp"
#include "lifting/prune_null_succs.hpp"
#include "lifting/prune_trivially_dead_succs.hpp"
#include "lifting/recover_functions.hpp"
#include "lifting/remove_metadata.hpp"
#include "lifting/remove_opt_none.hpp"
#include "lifting/remove_s2e_helpers.hpp"
#include "lifting/replace_dynamic_symbols.hpp"
#include "lifting/successor_lists.hpp"
#include "lifting/unalign_stack.hpp"
#include "lowering/halt_on_declarations.hpp"
#include "lowering/internalize_debug_helpers.hpp"
#include "lowering/remove_sections.hpp"
#include "merging/decompose_env.hpp"
#include "merging/externalize_functions.hpp"
#include "merging/internalize_imports.hpp"
#include "merging/rename_block_funcs.hpp"
#include "merging/rename_env.hpp"
#include "merging/unflatten_env.hpp"
#include "merging/unimplement_custom_helpers.hpp"
#include "object/function_renaming.hpp"
#include "set_data_layout_32.hpp"
#include "tag_inst_pc.hpp"
#include "utils/intrinsic_cleaner.hpp"
#include "utils/name_cleaner.hpp"
#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/GlobalsModRef.h>
#include <llvm/Analysis/MemorySSA.h>
#include <llvm/Analysis/OptimizationRemarkEmitter.h>
#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/IPO/GlobalDCE.h>
#include <llvm/Transforms/IPO/GlobalOpt.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar/DCE.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/LICM.h>

using namespace binrec;
using namespace llvm;
using namespace llvm::cl;
using namespace std;

namespace {
    opt<string> Trace_Filename{
        Positional,
        desc{"<trace bitcode file>"},
        init("-"),
        value_desc{"filename"}};
    opt<string> Output_Filename{"o", desc{"Output filename"}, value_desc{"filename"}};

    opt<bool> Link_Prep_1{
        "link-prep-1",
        desc{"Prepare trace for linking with other traces phase 1"}};
    opt<bool> Link_Prep_2{
        "link-prep-2",
        desc{"Prepare trace for linking with other traces phase 2"}};
    opt<bool> Clean{"clean", desc{"Clean trace for linking with custom helpers"}};
    opt<bool> Lift{"lift", desc{"Lift trace into correct LLVM module"}};
    opt<bool> Optimize{"optimize", desc{"Optimize module"}};
    opt<bool> Optimize_Better{"optimize-better", desc{"Optimize module better"}};
    opt<bool> Compile{"compile", desc{"Compile the trace to an object file"}};

    opt<bool> No_Link_Lift{"no-link-lift", desc{"Do not lift dynamic symbols"}};
    opt<bool> Clean_Names{"clean-names", desc{"Do not lift dynamic symbols"}};

    opt<bool> Trace_Calls{
        "trace-calls",
        desc{"Trace calls and register values of recovered functions"}};

    auto build_pipeline(PassBuilder &pb) -> ModulePassManager
    {
        ModulePassManager mpm;

        if (Link_Prep_1) {
            mpm.addPass(RenameBlockFuncsPass{});
            mpm.addPass(ExternalizeFunctionsPass{});
            mpm.addPass(InternalizeImportsPass{});
            mpm.addPass(GlobalDCEPass{});
        }

        if (Link_Prep_2) {
            mpm.addPass(InternalizeImportsPass{});
            mpm.addPass(UnimplementCustomHelpersPass{});
            mpm.addPass(GlobalDCEPass{});
            mpm.addPass(UnflattenEnvPass{});
            mpm.addPass(DecomposeEnvPass{});
            mpm.addPass(RenameEnvPass{});
        }

        if (Clean) {
            mpm.addPass(createModuleToFunctionPassAdaptor(InstCombinePass{}));
            mpm.addPass(TagInstPcPass{});
            mpm.addPass(RemoveS2EHelpersPass{});
            mpm.addPass(createModuleToFunctionPassAdaptor(DCEPass{}));
            mpm.addPass(AddCustomHelperVarsPass{});
            mpm.addPass(SetDataLayout32Pass{});
        }

        if (Lift) {
            mpm.addPass(RemoveOptNonePass{});
            mpm.addPass(InternalizeGlobalsPass{});
            mpm.addPass(GlobalDCEPass{});
            mpm.addPass(SuccessorListsPass{});
            mpm.addPass(ELFSymbolsPass{});
            mpm.addPass(PruneNullSuccsPass{});
            mpm.addPass(RecoverFunctionsPass{});
            mpm.addPass(FixOverlapsPass{});
            mpm.addPass(PruneTriviallyDeadSuccsPass{});
            mpm.addPass(InsertCallsPass{});
            mpm.addPass(AddMemArrayPass{});
            mpm.addPass(createModuleToFunctionPassAdaptor(ExternPLTPass{}));
            mpm.addPass(AddDebugPass{});
            mpm.addPass(AlwaysInlinerPass{});
            mpm.addPass(InlineStubsPass{});
            mpm.addPass(PcJumpsPass{});
            mpm.addPass(FixCFGPass{});
            mpm.addPass(InsertTrampForRecFuncsPass{});
            mpm.addPass(InlineLibCallArgsPass{});
            mpm.addPass(createModuleToFunctionPassAdaptor(DCEPass{}));
            mpm.addPass(AlwaysInlinerPass{});
            if (!No_Link_Lift) {
                mpm.addPass(ReplaceDynamicSymbolsPass{});
                mpm.addPass(LibCallNewPLTPass{});
                mpm.addPass(ImplementLibCallsNewPLTPass{});
            } else {
                mpm.addPass(ImplementLibCallStubsPass{});
            }
            mpm.addPass(InlineQemuOpHelpersPass{});
            mpm.addPass(GlobalEnvToAllocaPass{});
            if (Trace_Calls) {
                mpm.addPass(CallTracerPass{});
            }
            mpm.addPass(createModuleToFunctionPassAdaptor(InternalizeFunctionsPass{}));
            mpm.addPass(GlobalDCEPass{});
            mpm.addPass(UnalignStackPass{});
            mpm.addPass(RemoveMetadataPass{});
            mpm.addPass(GlobalDCEPass{});
            mpm.addPass(createModuleToFunctionPassAdaptor(DCEPass{}));
            mpm.addPass(IntrinsicCleanerPass{});
            mpm.addPass(FunctionRenamingPass{});
        }

        if (Optimize) {
            mpm.addPass(pb.buildPerModuleDefaultPipeline(PassBuilder::OptimizationLevel::O3));
        }

        if (Optimize_Better) {
            // FPar: I took this from the old optimizerBetter.sh script.
            mpm.addPass(RequireAnalysisPass<GlobalsAA, Module>{});
            mpm.addPass(createModuleToFunctionPassAdaptor(
                RequireAnalysisPass<OptimizationRemarkEmitterAnalysis, Function>{}));
            mpm.addPass(
                createModuleToFunctionPassAdaptor(createFunctionToLoopPassAdaptor(LICMPass{})));
            mpm.addPass(InlineWrapperPass{});
            mpm.addPass(createModuleToFunctionPassAdaptor(DCEPass{}));
            mpm.addPass(AlwaysInlinerPass{});
            mpm.addPass(createModuleToFunctionPassAdaptor(DCEPass{}));
            mpm.addPass(createModuleToFunctionPassAdaptor(GVN{}));
            mpm.addPass(createModuleToFunctionPassAdaptor(DCEPass{}));
            mpm.addPass(pb.buildModuleOptimizationPipeline(PassBuilder::OptimizationLevel::O3));
            mpm.addPass(GlobalOptPass{});
        }

        if (Compile) {
            mpm.addPass(HaltOnDeclarationsPass{});
            mpm.addPass(RemoveSectionsPass{});
            mpm.addPass(createModuleToFunctionPassAdaptor(InternalizeDebugHelpersPass{}));
            mpm.addPass(GlobalDCEPass{});
        }

        if (Clean_Names) {
            mpm.addPass(createModuleToFunctionPassAdaptor(NameCleaner{}));
        }

        return mpm;
    }
} // namespace

auto main(int argc, char *argv[]) -> int
{
    InitLLVM init_llvm{argc, argv};
    LLVMContext llvm_context;

    ParseCommandLineOptions(argc, argv, "BinRec trace lifter and optimizer\n");

    SMDiagnostic err;
    unique_ptr<Module> module = parseIRFile(Trace_Filename, err, llvm_context);
    if (!module) {
        err.print(argv[0], errs());
        return 1;
    }
    Triple triple(module->getTargetTriple());

    PassBuilder pb;

    AAManager aa = pb.buildDefaultAAPipeline();
    aa.registerFunctionAnalysis<EnvAa>();

    LoopAnalysisManager lam;
    FunctionAnalysisManager fam;
    fam.registerPass([] { return EnvAa{}; });
    CGSCCAnalysisManager cgam;
    ModuleAnalysisManager mam;

    mam.registerPass([] { return TraceInfoAnalysis{}; });
    fam.registerPass([&] { return move(aa); });

    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.crossRegisterProxies(lam, fam, cgam, mam);

    ModulePassManager mpm = build_pipeline(pb);

    error_code ec;
    raw_fd_ostream output_bc{Output_Filename + ".bc", ec};
    if (ec) {
        errs() << ec.message();
        return 1;
    }
    raw_fd_ostream output_ll{Output_Filename + ".ll", ec};
    if (ec) {
        errs() << ec.message();
        return 1;
    }
    mpm.addPass(BitcodeWriterPass{output_bc});
    mpm.addPass(PrintModulePass{output_ll});

    raw_fd_ostream memssa_ll{Output_Filename + "-memssa.ll", ec};
    if (ec) {
        errs() << ec.message();
        return 1;
    }
    mpm.addPass(RequireAnalysisPass<GlobalsAA, Module>{});
    mpm.addPass(createModuleToFunctionPassAdaptor(MemorySSAPrinterPass{memssa_ll}));

    mpm.run(*module, mam);

    return 0;
}
