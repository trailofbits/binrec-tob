/*
 * for each stub address in the symbols file:
 * 1. find the corresponding function and annotate its entry block with
 *    "extern_symbol" metadata
 * 2. remove the first successor and its successor, which are PLT code, copying
 *    the successors of the latter to the stub block and remove both BBs.
 */

#include "elf_symbols.hpp"
#include "ir/selectors.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include <algorithm>
#include <fstream>
#include <set>

using namespace binrec;
using namespace llvm;
using namespace std;

static void annotate_symbol(Function *f, const string &symbol)
{
    assert(!f->empty() && "stub function is empty");
    LLVMContext &ctx = f->getContext();
    MDNode *md = MDNode::get(ctx, MDString::get(ctx, symbol));
    setBlockMeta(f, "extern_symbol", md);
}

static auto get_fall_through_succ(Function *f) -> Function *
{
    Function *next = nullptr;
    unsigned lowest_pc = -1, block_pc = getBlockAddress(f);

    vector<Function *> succs;
    getBlockSuccs(f, succs);

    // get the block with the lowest PC that is still higher than that of f
    for (Function *succ : succs) {
        unsigned succ_pc = getBlockAddress(succ);
        if (succ_pc > block_pc && succ_pc < lowest_pc) {
            next = succ;
            lowest_pc = succ_pc;
        }
    }

    // this assumes a 6-byte stub containing a jmp instruction
    return lowest_pc - block_pc == 6 ? next : nullptr;
}

static auto remove_plt_succs(Function *f, set<Function *> &erase_list) -> bool
{
    vector<Function *> succs;
    unsigned nsuccs = getBlockMeta(f, "succs")->getNumOperands();

    assert(nsuccs >= 1);
    Function *ft = get_fall_through_succ(f);

    if (!ft) {
        if (logLevel >= logging::DEBUG) {
            raw_ostream &log = logging::getStream(logging::DEBUG);
            log << "no fallthrough successor: " << utohexstr(getBlockAddress(f)) << " -> ";
            getBlockSuccs(f, succs);
            bool first = true;
            for (Function *succ : succs) {
                if (!first)
                    log << ", ";
                log << utohexstr(getBlockAddress(succ));
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

    DBG("removing PLT code " << ft->getName() << " and " << plt->getName() << " for stub "
                             << f->getName());

    // ignore `ft` because it will be removed, and append all successors of
    // `plt` instead
    failUnless(getBlockSuccs(f, succs));
    assert(vectorContains(succs, ft));

    vector<Function *> merged;
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

    erase_list.insert(plt);
    erase_list.insert(ft);

    return true;
}

// NOLINTNEXTLINE
auto ELFSymbolsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    bool changed = false;

    ifstream f;
    s2eOpen(f, "symbols");
    DBG("called ELFSYMBOLS");
    uint64_t addr = 0;
    string symbol;
    while (f >> hex >> addr >> symbol) {
        if (Function *f2 = m.getFunction("Func_" + utohexstr(addr))) {
            annotate_symbol(f2, symbol);
            changed = true;
        }
    }

    set<Function *> erase_list;
    for (Function &f3 : LiftedFunctions{m}) {
        if (getBlockMeta(&f3, "extern_symbol"))
            remove_plt_succs(&f3, erase_list);
    }

    for (auto &it : erase_list)
        it->eraseFromParent();

    return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
