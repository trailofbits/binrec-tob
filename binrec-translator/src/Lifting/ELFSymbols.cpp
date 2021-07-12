/*
 * for each stub address in the symbols file:
 * 1. find the corresponding function and annotate its entry block with
 *    "extern_symbol" metadata
 * 2. remove the first successor and its successor, which are PLT code, copying
 *    the successors of the latter to the stub block and remove both BBs.
 */

#include "IR/Selectors.h"
#include "MetaUtils.h"
#include "PassUtils.h"
#include "PcUtils.h"
#include "RegisterPasses.h"
#include <algorithm>
#include <fstream>
#include <llvm/IR/PassManager.h>
#include <set>

using namespace binrec;
using namespace llvm;

namespace {
void annotateSymbol(Function *f, std::string symbol) {
    assert(!f->empty() && "stub function is empty");
    LLVMContext &ctx = f->getContext();
    MDNode *md = MDNode::get(ctx, MDString::get(ctx, symbol));
    setBlockMeta(f, "extern_symbol", md);
}

auto getFallThroughSucc(Function *f) -> Function * {
    Function *next = nullptr;
    unsigned lowestPc = -1, blockPc = getBlockAddress(f);

    std::vector<Function *> succs;
    getBlockSuccs(f, succs);

    // get the block with the lowest PC that is still higher than that of f
    for (Function *succ : succs) {
        unsigned succPc = getBlockAddress(succ);
        if (succPc > blockPc && succPc < lowestPc) {
            next = succ;
            lowestPc = succPc;
        }
    }

    // this assumes a 6-byte stub containing a jmp instruction
    return lowestPc - blockPc == 6 ? next : nullptr;
}

auto removePLTSuccs(Function *f, std::set<Function *> &eraseList) -> bool {
    std::vector<Function *> succs;
    unsigned nsuccs = getBlockMeta(f, "succs")->getNumOperands();

    assert(nsuccs >= 1);
    Function *ft = getFallThroughSucc(f);

    if (!ft) {
        if (logLevel >= logging::DEBUG) {
            raw_ostream &log = logging::getStream(logging::DEBUG);
            log << "no fallthrough successor: " << intToHex(getBlockAddress(f)) << " -> ";
            getBlockSuccs(f, succs);
            bool first = true;
            for (Function *succ : succs) {
                if (!first)
                    log << ", ";
                log << intToHex(getBlockAddress(succ));
                first = false;
            }
            errs() << "\n";
        }

        return false;
    }

    // linking part of stub should do a durect jump to linking code at the
    // start of the PLT
    getBlockSuccs(ft, succs);
    assert(succs.size() == 1);
    Function *plt = succs[0];

    DBG("removing PLT code " << ft->getName() << " and " << plt->getName() << " for stub " << f->getName());

    // ignore `ft` because it will be removed, and append all successors of
    // `plt` instead
    failUnless(getBlockSuccs(f, succs));
    assert(vectorContains(succs, ft));

    std::vector<Function *> merged;
    for (Function *s : succs) {
        if (s != ft)
            merged.push_back(s);
    }
    failUnless(getBlockSuccs(plt, succs));
    for (Function *s : succs) {
        if (!vectorContains(merged, s))
            merged.push_back(s);
    }
    setBlockSuccs(f, merged);

    eraseList.insert(plt);
    eraseList.insert(ft);

    return true;
}

/// S2E Add symbol metadata for ELF files based on stub addresses
class ELFSymbolsPass : public PassInfoMixin<ELFSymbolsPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        bool changed = false;

        std::ifstream f;
        s2eOpen(f, "symbols");
        DBG("called ELFSYMBOLS");
        uint64_t addr = 0;
        std::string symbol;
        while (f >> std::hex >> addr >> symbol) {
            if (Function *f = m.getFunction("Func_" + intToHex(addr))) {
                annotateSymbol(f, symbol);
                changed = true;
            }
        }

        std::set<Function *> eraseList;
        for (Function &f : LiftedFunctions{m}) {
            if (getBlockMeta(&f, "extern_symbol"))
                removePLTSuccs(&f, eraseList);
        }

        for (auto &it : eraseList)
            it->eraseFromParent();

        return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
};
} // namespace

void binrec::addELFSymbolsPass(ModulePassManager &mpm) { mpm.addPass(ELFSymbolsPass()); }
