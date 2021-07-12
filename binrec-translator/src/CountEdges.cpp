#include "CountEdges.h"
#include "PassUtils.h"
#include <algorithm>
#include <fstream>
#include <llvm/Support/FileSystem.h>
#include <set>
#include <unordered_set>

using namespace llvm;

char CountEdges::ID = 0;
static RegisterPass<CountEdges> x("count-edges", "S2E Count indirect edges in recovered code", false, false);

static cl::opt<std::string> OutFile("edges-outfile", cl::desc("Path to write edge counts to"), cl::value_desc("path"),
                                    cl::init("edges.dat"));

static cl::opt<bool> Verbose("edges-verbose", cl::desc("Show edge count aggregated"), cl::init(false));

auto CountEdges::runOnModule(Module &m) -> bool {
    std::vector<unsigned> nsuccs;
    Function *f = nullptr;

    if (!(f = m.getFunction("Func_wrapper"))) {
        if (!(f = m.getFunction("main")))
            assert(false);
    }

    Function *trampoline = m.getFunction("helper_stub_trampoline");
    assert(trampoline && "trampoline not found");

    std::unordered_set<BasicBlock *> trampBBs;
    // iterate over lib functions
    for (auto *U : trampoline->users()) {
        auto *I = cast<Instruction>(U);
        BasicBlock *BB = I->getParent();
        trampBBs.insert(BB);
    }

    std::ifstream ff;
    s2eOpen(ff, "entryToReturn");
    std::unordered_set<uint32_t> retBBs;
    int32_t func_start = 0, func_end = 0;
    while (ff.read((char *)&func_end, 4).read((char *)&func_start, 4)) {
        // errs() << "func_start: " << intToHex(func_start) << " func_end: " << intToHex(func_end) << "\n";
        // errs() << "func_start: " << func_start << " func_end: " << func_end << "\n";

        // entryPcToReturnPcs[func_start].insert(func_end);
        // std::stringstream stream;
        // stream << std::hex << func_end;
        retBBs.insert(func_end);
    }

    for (BasicBlock &bb : *f) {
        // bbs that makes external calls
        if (trampBBs.find(&bb) != trampBBs.end())
            continue;

        uint32_t pc = 0;
        std::string hex = " ";
        if (bb.getName().startswith("BB_")) {
            hex = bb.getName().substr(3);
            pc = hexToInt(hex);
        } else if (bb.getName().startswith("Func_")) {
            hex = bb.getName().substr(5, bb.getName().rfind(".") - 5);
            pc = hexToInt(hex);
        } else {
            hex = "Error";
        }

        // return bbs of local functions
        if (retBBs.find(pc) != retBBs.end())
            continue;

        Instruction *term = bb.getTerminator();
        assert(term);

        if (auto *sw = dyn_cast<SwitchInst>(term)) {
            unsigned n = sw->getNumSuccessors();
            errs() << bb.getName() << " " << hex << " " << n << '\n';
            if (sw->getDefaultDest()->getName() == "error")
                n--;
            nsuccs.push_back(n);
        }
    }

    std::sort(nsuccs.begin(), nsuccs.end());

    std::error_code EC;
    raw_fd_ostream os(OutFile.getValue().c_str(), EC, sys::fs::OF_Text);

    if (os.has_error()) {
        ERROR("could not open " << OutFile << ": " << EC.message());
        exit(1);
    }

    for (const unsigned n : nsuccs)
        os << n << "\n";

    if (Verbose) {
        std::map<unsigned, unsigned> smap;

        for (const unsigned n : nsuccs) {
            if (smap.find(n) == smap.end())
                smap[n] = 1;
            else
                smap[n]++;
        }

        outs() << nsuccs.size() << " switch statements\n";

        for (const auto &it : smap)
            outs() << it.first << ": " << it.second << "\n";
    }

    return false;
}
