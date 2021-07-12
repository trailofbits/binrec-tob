#include "InstrumentInterpreter.h"
#include "DebugUtils.h"
#include "PassUtils.h"
#include "UntangleInterpreter.h"

using namespace llvm;

char InstrumentInterpreter::ID = 0;
static RegisterPass<InstrumentInterpreter>
    X("instrument-interpreter", "S2E Instrument interpreter loop to print virtual program counter", false, false);

auto InstrumentInterpreter::doInitialization(Module &m) -> bool {
    bool HaveErr = false;
    if (InterpreterEntry.getNumOccurrences() != 1) {
        errs() << "error: please specify one -interpreter-entry\n";
        HaveErr = true;
    }

    if (VpcGlobal.getNumOccurrences() != 1) {
        errs() << "error: please specify one -interpreter-vpc\n";
        HaveErr = true;
    }

    if (HaveErr)
        exit(1);

    return false;
}

static auto findInterpreterEntryBlock(Function &wrapper) -> BasicBlock * {
    for (BasicBlock &bb : wrapper) {
        if (bb.hasName() && bb.getName() == InterpreterEntry)
            return &bb;
    }
    return nullptr;
}

auto InstrumentInterpreter::runOnModule(Module &m) -> bool {
    GlobalVariable *vpc = m.getNamedGlobal(VpcGlobal);
    assert(vpc && "no vpc found");
    Function *wrapper = m.getFunction("Func_wrapper");
    assert(wrapper && "no wrapper found");
    BasicBlock *entryBlock = findInterpreterEntryBlock(*wrapper);
    assert(entryBlock && "no entry block found");

    IRBuilder<> b(&*(entryBlock->begin()));
    buildWrite(b, "vpc:");
    buildWriteUint(b, b.CreateLoad(vpc));
    buildWriteChar(b, '\n');

    return true;
}
