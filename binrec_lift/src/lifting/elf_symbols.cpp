/*
 * for each stub address in the symbols file:
 * 1. find the corresponding function and annotate its entry block with
 *    "extern_symbol" metadata
 * 2. remove the first successor (lookup block) and its successor (entry resolution),
 *    which are PLT code, copying the successors of the latter to the lookup block
 *    and remove both BBs.
 */

#include "elf_symbols.hpp"
#include "error.hpp"
#include "ir/selectors.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include <algorithm>
#include <fstream>
#include <set>

#define PASS_NAME "elf_symbols"
#define PASS_ASSERT(cond) LIFT_ASSERT(PASS_NAME, cond)

using namespace binrec;
using namespace llvm;
using namespace std;


namespace {
    struct elf_section {
        string name;
        unsigned address;
        unsigned size;
    };
} // namespace

static void annotate_symbol(Function *f, const string &symbol)
{
    PASS_ASSERT(!f->empty() && "stub function is empty");
    LLVMContext &ctx = f->getContext();
    MDNode *md = MDNode::get(ctx, MDString::get(ctx, symbol));
    setBlockMeta(f, "extern_symbol", md);
}

static auto get_plt_lookup_function(Function *f, const elf_section &plt_section) -> Function *
{
    Function *next = nullptr;
    unsigned block_pc = getBlockAddress(f);
    vector<Function *> succs;
    unsigned plt_end = plt_section.address + plt_section.size;

    getBlockSuccs(f, succs);

    // If the PLT was not resolved there might not be any successors, however this should not be the
    // case.
    if (succs.empty()) {
        return nullptr;
    }

    for (Function *succ : succs) {
        // Filter to only successors that are located within the .plt section and ignore
        // self-referential successors. See Github issue #168 for more information,
        // https://github.com/trailofbits/binrec-prerelease/issues/168
        unsigned succ_pc = getBlockAddress(succ);
        if (succ_pc >= plt_section.address && succ_pc < plt_end && succ_pc != block_pc) {
            if (next) {
                errs() << "[ElfSymbols] Unable to determine successor of PLT " << f->getName()
                       << " because ther are multiple successors located within the .plt section\n";
                return nullptr;
            }

            next = succ;
        }
    }
    return next;
}

// This method removes the two successors from a library function call via the
// procedure load table (PLT). The first time a library function is called, the
// successor call chain will look like the following:
//
//   - Function: Func_80490C0 (printf@plt)
//     - PLT lookup Function: Func_8049040
//       - PLT Entry: Func_8049030
//         - Function: printf in libc
//
// This method removes the lookup and PLT entry functions, both of which are
// located within the ELF ".plt" section.
//
// Subsequent calls into the library function call directly into the library
// because the address is cached in the PLT.
static auto
remove_plt_succs(Function *f, set<Function *> &erase_list, const elf_section &plt_section) -> bool
{
    vector<Function *> succs;
    unsigned nsuccs = getBlockMeta(f, "succs")->getNumOperands();

    PASS_ASSERT(nsuccs >= 1);
    Function *lookup = get_plt_lookup_function(f, plt_section);

    if (!lookup) {
        if (logLevel >= logging::DEBUG) {
            raw_ostream &log = logging::getStream(logging::DEBUG);
            log << "no plt successor: " << utohexstr(getBlockAddress(f)) << " -> ";
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

    // linking part of stub should do a direct jump to linking code at the
    // start of the PLT
    getBlockSuccs(lookup, succs);
    PASS_ASSERT(succs.size() == 1);
    Function *plt = succs[0];

    DBG("removing PLT code " << lookup->getName() << " and " << plt->getName() << " for stub "
                             << f->getName());

    // ignore `lookup` because it will be removed, and append all successors of
    // `plt` instead
    failUnless(getBlockSuccs(f, succs));
    PASS_ASSERT(vectorContains(succs, lookup));

    vector<Function *> merged;
    for (Function *s : succs) {
        if (s != lookup)
            merged.push_back(s);
    }
    failUnless(getBlockSuccs(plt, succs));
    for (Function *s : succs) {
        if (!vectorContains(merged, s))
            merged.push_back(s);
    }
    setBlockSuccs(f, merged);

    erase_list.insert(plt);
    erase_list.insert(lookup);

    return true;
}

// NOLINTNEXTLINE
auto ELFSymbolsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    DBG("Running ELF Symbols pass");
    bool changed = false;

    // load the external symbols
    ifstream f;
    s2eOpen(f, "symbols");
    uint64_t addr = 0;
    unsigned size = 0;
    string symbol;
    while (f >> hex >> addr >> symbol) {
        if (Function *f2 = m.getFunction("Func_" + utohexstr(addr))) {
            annotate_symbol(f2, symbol);
            changed = true;
        }
    }
    f.close();

    // load the .plt section info
    elf_section plt_section{".plt", 0, 0};
    s2eOpen(f, "sections");
    while (f >> hex >> addr >> hex >> size >> symbol) {
        if (symbol == ".plt") {
            plt_section.address = addr;
            plt_section.size = size;
        }
    }
    f.close();

    // bail early if there are external functions but no .plt section.
    PASS_ASSERT((!changed || plt_section.address) && "binary does not contain .plt section");

    set<Function *> erase_list;
    for (Function &f3 : LiftedFunctions{m}) {
        if (getBlockMeta(&f3, "extern_symbol"))
            remove_plt_succs(&f3, erase_list, plt_section);
    }

    for (auto &it : erase_list)
        it->eraseFromParent();

    return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
