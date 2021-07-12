#include "AddCustomHelperVars.h"
#include "Analysis/EnvAliasAnalysis.h"
#include "Analysis/TraceInfoAnalysis.h"
#include "Debug/CallTracer.h"
#include "IR/Selectors.h"
#include "InlineWrapper.h"
#include "Lifting/AddMemArray.h"
#include "Lifting/GlobalEnvToAlloca.h"
#include "Lifting/InlineLibCallArgs.h"
#include "Lifting/RemoveOptNone.h"
#include "Lifting/RemoveS2EHelpers.h"
#include "Lowering/RemoveSections.h"
#include "Object/FunctionRenaming.h"
#include "RegisterPasses.h"
#include "SetDataLayout32.h"
#include "TagInstPc.h"
#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/GlobalsModRef.h>
#include <llvm/Analysis/MemorySSA.h>
#include <llvm/Analysis/OptimizationRemarkEmitter.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
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
opt<string> traceFilename{Positional, desc{"<trace bitcode file>"}, init("-"), value_desc{"filename"}};
opt<string> outputFilename{"o", desc{"Output filename"}, value_desc{"filename"}};

opt<bool> clean{"clean", desc{"Clean trace for linking with custom helpers"}};
opt<bool> lift{"lift", desc{"Lift trace into correct LLVM module"}};
opt<bool> symstack{"symstack", desc{"Lift trace into correct LLVM module"}};
opt<bool> optimize{"optimize", desc{"Optimize module"}};
opt<bool> optimizeBetter{"optimize-better", desc{"Optimize module better"}};
opt<bool> compile{"compile", desc{"Compile the trace to an object file"}};

opt<bool> noLinkLift{"no-link-lift", desc{"Do not lift dynamic symbols"}};

opt<bool> printDsi{"print-dsi", desc{"Print results of DSI analysis"}};

opt<bool> instrumentStackSize("instrument-stack-size", desc{"Instrument the code to find max stack sizes"});
opt<bool> traceCalls("trace-calls", desc{"Trace calls and register values of recovered functions"});

auto buildPipeline(PassBuilder &pb) -> ModulePassManager {
    ModulePassManager mpm;

    if (clean) {
        mpm.addPass(createModuleToFunctionPassAdaptor(InstCombinePass{}));
        mpm.addPass(TagInstPcPass{});
        mpm.addPass(RemoveS2EHelpersPass{});
        mpm.addPass(createModuleToFunctionPassAdaptor(DCEPass{}));
        mpm.addPass(AddCustomHelperVarsPass{});
        mpm.addPass(SetDataLayout32Pass{});
    }

    if (lift) {
        mpm.addPass(RemoveOptNonePass{});
        addInternalizeGlobalsPass(mpm);
        mpm.addPass(GlobalDCEPass{});
        addSuccessorListsPass(mpm);
        addELFSymbolsPass(mpm);
        addPruneNullSuccsPass(mpm);
        addRecoverFunctionsPass(mpm);
        addFixOverlapsPass(mpm);
        addPruneTriviallyDeadSuccsPass(mpm);
        addInsertCallsPass(mpm);
        mpm.addPass(AddMemArrayPass{});
        addExternPLTPass(mpm);
        addAddDebugPass(mpm);
        mpm.addPass(AlwaysInlinerPass{});
        addInlineStubsPass(mpm);
        addPcJumpsPass(mpm);
        addFixCFGPass(mpm);
        addInsertTrampForRecFuncsPass(mpm);
        mpm.addPass(InlineLibCallArgsPass{});
        mpm.addPass(createModuleToFunctionPassAdaptor(DCEPass{}));
        mpm.addPass(AlwaysInlinerPass{});
        if (!noLinkLift) {
            addReplaceDynamicSymbolsPass(mpm);
            addLibCallNewPLTPass(mpm);
            addImplementLibCallsNewPLTPass(mpm);
        } else {
            addImplementLibCallStubsPass(mpm);
        }
        addInlineQemuOpHelpersPass(mpm);
        mpm.addPass(GlobalEnvToAllocaPass{});
        if (traceCalls) {
            mpm.addPass(CallTracerPass{});
        }
        addInternalizeFunctionsPass(mpm);
        mpm.addPass(GlobalDCEPass{});
        addUnalignStackPass(mpm);
        addRemoveMetadataPass(mpm);
        mpm.addPass(GlobalDCEPass{});
        mpm.addPass(createModuleToFunctionPassAdaptor(DCEPass{}));
        mpm.addPass(FunctionRenamingPass{});
    }

    if (optimize) {
        mpm.addPass(pb.buildPerModuleDefaultPipeline(PassBuilder::OptimizationLevel::O3));
    }

    if (optimizeBetter) {
        // FPar: I took this from the old optimizerBetter.sh script.
        mpm.addPass(RequireAnalysisPass<GlobalsAA, Module>{});
        mpm.addPass(
            createModuleToFunctionPassAdaptor(RequireAnalysisPass<OptimizationRemarkEmitterAnalysis, Function>{}));
        mpm.addPass(createModuleToFunctionPassAdaptor(createFunctionToLoopPassAdaptor(LICMPass{})));
        mpm.addPass(InlineWrapperPass{});
        mpm.addPass(createModuleToFunctionPassAdaptor(DCEPass{}));
        mpm.addPass(AlwaysInlinerPass{});
        mpm.addPass(createModuleToFunctionPassAdaptor(DCEPass{}));
        mpm.addPass(createModuleToFunctionPassAdaptor(GVN{}));
        mpm.addPass(createModuleToFunctionPassAdaptor(DCEPass{}));
        mpm.addPass(pb.buildModuleOptimizationPipeline(PassBuilder::OptimizationLevel::O3));
        mpm.addPass(GlobalOptPass{});
    }

    if (compile) {
        addHaltOnDeclarationsPass(mpm);
        mpm.addPass(RemoveSectionsPass{});
        addInternalizeDebugHelpersPass(mpm);
        mpm.addPass(GlobalDCEPass{});
    }

    return mpm;
}
} // namespace

auto main(int argc, char *argv[]) -> int {
    InitLLVM initLLVM{argc, argv};
    LLVMContext llvmContext;

    ParseCommandLineOptions(argc, argv, "BinRec trace lifter and optimizer\n");

    SMDiagnostic err;
    unique_ptr<Module> module = parseIRFile(traceFilename, err, llvmContext);
    if (!module) {
        err.print(argv[0], errs());
        return 1;
    }
    Triple triple(module->getTargetTriple());

    PassBuilder pb;

    AAManager aa = pb.buildDefaultAAPipeline();
    aa.registerFunctionAnalysis<EnvAA>();

    LoopAnalysisManager lam;
    FunctionAnalysisManager fam;
    fam.registerPass([] { return EnvAA{}; });
    CGSCCAnalysisManager cgam;
    ModuleAnalysisManager mam;

    fam.registerPass([=] {
        TargetLibraryInfoImpl tlii(triple);
        tlii.disableAllFunctions();
        return TargetLibraryAnalysis{move(tlii)};
    });

    mam.registerPass([] { return TraceInfoAnalysis{}; });
    fam.registerPass([&] { return move(aa); });

    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.crossRegisterProxies(lam, fam, cgam, mam);

    ModulePassManager mpm = buildPipeline(pb);

    error_code ec;
    raw_fd_ostream outputBC{outputFilename + ".bc", ec};
    if (ec) {
        errs() << ec.message();
        return 1;
    }
    raw_fd_ostream outputLL{outputFilename + ".ll", ec};
    if (ec) {
        errs() << ec.message();
        return 1;
    }
    mpm.addPass(BitcodeWriterPass{outputBC});
    mpm.addPass(PrintModulePass{outputLL});

    raw_fd_ostream memssaLL{outputFilename + "-memssa.ll", ec};
    if (ec) {
        errs() << ec.message();
        return 1;
    }
    mpm.addPass(RequireAnalysisPass<GlobalsAA, Module>{});
    mpm.addPass(createModuleToFunctionPassAdaptor(MemorySSAPrinterPass{memssaLL}));

    mpm.run(*module, mam);

    return 0;
}
