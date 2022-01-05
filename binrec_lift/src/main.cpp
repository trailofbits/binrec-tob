#include "analysis/env_alias_analysis.hpp"
#include "analysis/trace_info_analysis.hpp"
#include "binrec_lift.hpp"
#include "lift_context.hpp"
#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/GlobalsModRef.h>
#include <llvm/Analysis/MemorySSA.h>
#include <llvm/Analysis/OptimizationRemarkEmitter.h>
#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>

using namespace llvm;
using namespace llvm::cl;
using namespace binrec;
using namespace std;

opt<string> Trace_Filename{
    Positional,
    desc{"<trace bitcode file>"},
    init("-"),
    value_desc{"filename"}};
opt<string> Output_Filename{"o", desc{"Output filename"}, value_desc{"filename"}};

opt<bool> Link_Prep_1{"link-prep-1", desc{"Prepare trace for linking with other traces phase 1"}};
opt<bool> Link_Prep_2{"link-prep-2", desc{"Prepare trace for linking with other traces phase 2"}};
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


auto main(int argc, char *argv[]) -> int
{
    InitLLVM init_llvm{argc, argv};
    LLVMContext llvm_context;
    LiftContext ctx;
    error_code ec;

    ParseCommandLineOptions(argc, argv, "BinRec trace lifter and optimizer\n");

    ctx.trace_filename = Trace_Filename;
    ctx.destination = Output_Filename;
    ctx.link_prep_1 = Link_Prep_1;
    ctx.link_prep_2 = Link_Prep_2;
    ctx.clean = Clean;
    ctx.lift = Lift;
    ctx.optimize = Optimize;
    ctx.optimize_better = Optimize_Better;
    ctx.compile = Compile;
    ctx.skip_link = No_Link_Lift;
    ctx.clean_names = Clean_Names;
    ctx.trace_calls = Trace_Calls;

    try {
        run_lift(ctx);
    } catch (runtime_error &error) {
        errs() << error.what() << "\n";
        return -1;
    }

    return 0;
}
