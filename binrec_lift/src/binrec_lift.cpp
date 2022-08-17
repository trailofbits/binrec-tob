#include "binrec_lift.hpp"
#include "add_custom_helper_vars.hpp"
#include "analysis/env_alias_analysis.hpp"
#include "analysis/trace_info_analysis.hpp"
#include "debug/call_tracer.hpp"
#include "error.hpp"
#include "inline_wrapper.hpp"
#include "ir/selectors.hpp"
#include "lifting/add_debug.hpp"
#include "lifting/add_mem_array.hpp"
#include "lifting/elf_symbols.hpp"
#include "lifting/extern_plt.hpp"
#include "lifting/fix_cfg.hpp"
#include "lifting/fix_overlaps.hpp"
#include "lifting/global_env_to_alloca.hpp"
#include "lifting/globalize_callback_addresses.hpp"
#include "lifting/globalize_data_imports.hpp"
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
#include "replace_local_function_pointers.hpp"
#include "lowering/halt_on_declarations.hpp"
#include "lowering/internalize_debug_helpers.hpp"
#include "lowering/remove_sections.hpp"
#include "merging/decompose_env.hpp"
#include "merging/externalize_functions.hpp"
#include "merging/globalize_env.hpp"
#include "merging/internalize_imports.hpp"
#include "merging/prune_redundant_basic_blocks.hpp"
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
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/IPO/GlobalDCE.h>
#include <llvm/Transforms/IPO/GlobalOpt.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar/DCE.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/LICM.h>
#include "lifting/debug_print_callback.hpp"

using namespace llvm;
using namespace llvm::cl;
using namespace std;

namespace binrec {


    auto build_pipeline(LiftContext &ctx, PassBuilder &pb) -> ModulePassManager
    {
        ModulePassManager mpm;

        if (ctx.link_prep_1) {
            mpm.addPass(RenameBlockFuncsPass{});
            mpm.addPass(ExternalizeFunctionsPass{});
            mpm.addPass(InternalizeImportsPass{});
            mpm.addPass(GlobalDCEPass{});
        }

        if (ctx.link_prep_2) {
            mpm.addPass(InternalizeImportsPass{});
            mpm.addPass(UnimplementCustomHelpersPass{});
            mpm.addPass(GlobalDCEPass{});
            mpm.addPass(GlobalizeEnvPass{});
            mpm.addPass(RenameEnvPass{});
        }

        if (ctx.clean) {
            mpm.addPass(createModuleToFunctionPassAdaptor(InstCombinePass{}));
            mpm.addPass(PruneRedundantBasicBlocksPass{});
            mpm.addPass(TagInstPcPass{});
            mpm.addPass(RemoveS2EHelpersPass{});
            mpm.addPass(createModuleToFunctionPassAdaptor(DCEPass{}));
            mpm.addPass(AddCustomHelperVarsPass{});
            mpm.addPass(SetDataLayout32Pass{});
        }

        if (ctx.lift) {
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
            mpm.addPass(GlobalizeCallbackAddresses{});
            // mpm.addPass(ReplaceLocalFunctionPointers{});
            mpm.addPass(DebugPrintCallback{"after_globalize", true});
            mpm.addPass(AddMemArrayPass{});
            // mpm.addPass(DebugPrintCallback{"after_mem_array"});
            mpm.addPass(createModuleToFunctionPassAdaptor(ExternPLTPass{}));
            // mpm.addPass(DebugPrintCallback{"after_extern_plt"});
            mpm.addPass(AddDebugPass{});
            mpm.addPass(AlwaysInlinerPass{});
            // mpm.addPass(DebugPrintCallback{"after_always_inline"});
            mpm.addPass(InlineStubsPass{});
            // mpm.addPass(DebugPrintCallback{"after_inline_stubs"});
            mpm.addPass(PcJumpsPass{});
            // mpm.addPass(DebugPrintCallback{"after_fix_cfg"});
            mpm.addPass(FixCFGPass{});
            mpm.addPass(InsertTrampForRecFuncsPass{});
            mpm.addPass(InlineLibCallArgsPass{});
            // mpm.addPass(DebugPrintCallback{"after_inline_lib_call"});
            mpm.addPass(createModuleToFunctionPassAdaptor(DCEPass{}));
            // mpm.addPass(DebugPrintCallback{"after_dce"});
            mpm.addPass(AlwaysInlinerPass{});
            // mpm.addPass(DebugPrintCallback{"after_always_inline_2"});
            if (!ctx.skip_link) {
                mpm.addPass(ReplaceDynamicSymbolsPass{});
                mpm.addPass(LibCallNewPLTPass{});
                mpm.addPass(ImplementLibCallsNewPLTPass{});
            } else {
                mpm.addPass(ImplementLibCallStubsPass{});
            }
            mpm.addPass(InlineQemuOpHelpersPass{});
            mpm.addPass(GlobalEnvToAllocaPass{});
            if (ctx.trace_calls) {
                mpm.addPass(CallTracerPass{});
            }
            // mpm.addPass(DebugPrintCallback{"after_ctx.skip_link"});
            mpm.addPass(createModuleToFunctionPassAdaptor(InternalizeFunctionsPass{}));
            mpm.addPass(DebugPrintCallback{"after_internalize_functions", true});
            mpm.addPass(ReplaceLocalFunctionPointers{});
            mpm.addPass(GlobalDCEPass{});
            mpm.addPass(DebugPrintCallback{"after_dce_2"});
            mpm.addPass(UnalignStackPass{});
            mpm.addPass(RemoveMetadataPass{});
            mpm.addPass(GlobalDCEPass{});
            mpm.addPass(createModuleToFunctionPassAdaptor(DCEPass{}));
            mpm.addPass(IntrinsicCleanerPass{});
            mpm.addPass(DebugPrintCallback{"after_intrinsic_cleaner"});
            mpm.addPass(FunctionRenamingPass{});
            mpm.addPass(DebugPrintCallback{"after_func_rename"});
            mpm.addPass(GlobalizeDataImportsPass{});
            mpm.addPass(DebugPrintCallback{"after_always_inline_2"});
        }

        if (ctx.optimize) {
            mpm.addPass(pb.buildPerModuleDefaultPipeline(OptimizationLevel::O3));
        }

        if (ctx.optimize_better) {
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
            mpm.addPass(createModuleToFunctionPassAdaptor(GVNPass{}));
            mpm.addPass(pb.buildModuleOptimizationPipeline(OptimizationLevel::O3));
            mpm.addPass(GlobalOptPass{});
        }

        if (ctx.compile) {
            mpm.addPass(HaltOnDeclarationsPass{});
            mpm.addPass(RemoveSectionsPass{});
            mpm.addPass(createModuleToFunctionPassAdaptor(InternalizeDebugHelpersPass{}));
            mpm.addPass(GlobalDCEPass{});
        }

        if (ctx.clean_names) {
            mpm.addPass(createModuleToFunctionPassAdaptor(NameCleaner{}));
        }

        return mpm;
    }

    void run_lift(LiftContext &ctx)
    {
        // This isn't ideal from the standpoint of a Python API. However, because
        // binrec_lift appears to be stack-based, breaking this function up may cause
        // segfaults as structs/classes go out of scope. This method is very fragile.
        LLVMContext llvm_context;
        SMDiagnostic err;
        unique_ptr<Module> module = parseIRFile(ctx.trace_filename, err, llvm_context);
        if (!module) {
            string error;
            raw_string_ostream error_stream{error};
            err.print("binrec-lift", error_stream);
            throw runtime_error{error};
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

        ModulePassManager mpm = build_pipeline(ctx, pb);

        error_code ec;
        raw_fd_ostream output_bc{ctx.destination + ".bc", ec};
        if (ec) {
            LLVM_ERROR(error) << "failed to open file " << ctx.destination
                              << ".bc: " << ec.message();
            throw runtime_error{error};
        }
        raw_fd_ostream output_ll{ctx.destination + ".ll", ec};
        if (ec) {
            LLVM_ERROR(error) << "failed to open file " << ctx.destination
                              << ".ll: " << ec.message();
            throw runtime_error{error};
        }
        mpm.addPass(BitcodeWriterPass{output_bc});
        mpm.addPass(PrintModulePass{output_ll});

        raw_fd_ostream memssa_ll{ctx.destination + "-memssa.ll", ec};
        if (ec) {
            LLVM_ERROR(error) << "failed to open file " << ctx.destination
                              << "-memssa.ll: " << ec.message();
            throw runtime_error{error};
        }
        mpm.addPass(RequireAnalysisPass<GlobalsAA, Module>{});
        mpm.addPass(createModuleToFunctionPassAdaptor(MemorySSAPrinterPass{memssa_ll}));

        mpm.run(*module, mam);
    }
} // namespace binrec
