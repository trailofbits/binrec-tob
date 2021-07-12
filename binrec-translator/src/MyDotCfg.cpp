#include "MyDotCfg.h"
#include "MetaUtils.h"
#include "PassUtils.h"
#include "PcUtils.h"
#include <algorithm>
#include <llvm/IR/CFG.h>
#include <llvm/Support/FileSystem.h>

using namespace llvm;

#define FILENAME "overlaps.dot"
#define ONLY_DIFF_END false
#define MAX_GROUPS 500

char MyDotCfg::ID = 0;
static RegisterPass<MyDotCfg> x("my-dot-cfg", "S2E Write dot cfg of @wrapper or @main", false, false);

static cl::opt<std::string> OutFile("cfg-outfile", cl::desc("Path to write dot cfg to"), cl::value_desc("path"),
                                    cl::init("cfg.dot"));

static cl::opt<unsigned> ScaleBlocks("cfg-scale-ratio", cl::desc("Instruction-to-block height scaling ratio"),
                                     cl::init(0));

static cl::opt<bool> ShowCode("cfg-show-code", cl::desc("Show LLVM code in dot graph"), cl::init(false));

static cl::opt<unsigned> MaxInstrWidth("cfg-max-instr-width",
                                       cl::desc("Maximum instruction width when using -cfg-show-code"), cl::init(0));

static auto blockId(std::map<BasicBlock *, unsigned> &bbMap, BasicBlock *bb) -> unsigned {
    auto it = bbMap.find(bb);

    if (it != bbMap.end())
        return it->second;

    unsigned id = bbMap.size();
    bbMap[bb] = id;
    return id;
}

static void printInst(raw_ostream &os, Instruction &inst) {
    std::string s;
    raw_string_ostream ss(s);
    ss << inst;
    ss.flush();
    for (unsigned i = 2, iUpperBound = s.size(); i < iUpperBound; ++i) {
        if (MaxInstrWidth && i - 2 == MaxInstrWidth) {
            os << " ...";
            break;
        }

        switch (s[i]) {
        case '"':
            os << "\\\"";
            break;
        case '\\':
            os << "\\\\";
            break;
        case '\t':
            os << "\\t";
            break;
        case '\n':
            os << "\\n";
            break;
        default:
            os << s[i];
        }
    }
    os << "\\l";
}

static void dotFunction(Function &f) {
    // std::ofstream ofile(OutFile);
    std::error_code EC;
    raw_fd_ostream ofile(OutFile.getValue().c_str(), EC, sys::fs::OF_Text);

    if (ofile.has_error()) {
        ERROR("could not open " << OutFile << ": " << EC.message());
        exit(1);
    }

    INFO("writing CFG to " << OutFile);

    std::map<BasicBlock *, unsigned> bbMap;

    ofile << "digraph " << f.getName() << " {\n";
    ofile << "margin = 0\n";
    ofile << "node [shape=box label=\"\"]\n";

    for (BasicBlock &bb : f) {
        ofile << blockId(bbMap, &bb);

        if (ShowCode) {
            ofile << " [label=\"";
            for (Instruction &i : bb)
                printInst(ofile, i);
            ofile << "\"]";
        } else if (ScaleBlocks) {
            ofile << R"( [label="\l\l)";
            for (unsigned i = 0, iUpperBound = bb.size() / ScaleBlocks; i < iUpperBound; ++i)
                ofile << "\\l";
            ofile << "\"]";
        }

        ofile << "\n";

        for (BasicBlock *succ : successors(&bb)) {
            {
                ofile << blockId(bbMap, &bb);
                if (ScaleBlocks)
                    ofile << ":s";
                ofile << " -> " << blockId(bbMap, succ);
                if (ScaleBlocks)
                    ofile << ":n";
                ofile << "\n";
            };
        }
    }

    ofile << "}\n";
    ofile.close();
}

auto MyDotCfg::runOnModule(Module &m) -> bool {
    if (Function *wrapper = m.getFunction("Func_wrapper")) {
        dotFunction(*wrapper);
    } else if (Function *mymain = m.getFunction("main")) {
        dotFunction(*mymain);
    } else {
        ERROR("No wrapper and no main function to dot Cfg for");
        exit(1);
    }

    return false;
}
