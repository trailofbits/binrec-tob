#include "name_cleaner.hpp"
#include "ir/selectors.hpp"

using namespace binrec;
using namespace llvm;

static void clean_bb_name(BasicBlock &bb)
{
    StringRef name = bb.getName();
    if (name.startswith("BB_")) {
        if (name.contains(".")) {
            bb.setName(name.substr(0, name.find_first_of(".")));
        } else {
            bb.setName(name);
        }
    } else if (name.contains(".")) {
        bb.setName("");
    }
}

static void clean_instruction_name(Instruction &i)
{
    if (!isa<AllocaInst>(i)) {
        StringRef name = i.getName();
        if (name.startswith("tmp")) {
            i.setName("");
        } else if (name.contains(".")) {
            i.setName(name.substr(0, name.find_first_of(".")));
        }
    }
}

// NOLINTNEXTLINE
auto NameCleaner::run(Function &f, FunctionAnalysisManager &am) -> PreservedAnalyses
{
    if (is_lifted_function(f)) {
        for (BasicBlock &bb : f) {
            clean_bb_name(bb);
            for (Instruction &i : bb) {
                clean_instruction_name(i);
            }
        }
    }

    return PreservedAnalyses::all();
}
