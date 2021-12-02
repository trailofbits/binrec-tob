#include <algorithm>
#include "tag_inst_pc.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include "ir/selectors.hpp"

using namespace binrec;
using namespace llvm;


namespace {
    void tag_pc_in_function(Function &f, GlobalVariable *pc, GlobalVariable *ret, GlobalVariable *ccdst, GlobalVariable *ccsrc, MDNode *md) {
        for (auto &bb : f) {
            for (auto &ins : bb) {
                if (auto store = dyn_cast<StoreInst>(&ins)) {
                    auto target = store->getPointerOperand();
                    // At this point we've hit a store to a 'register' (named global)
                    // that indicates the basic block ends and that the future PC-stores
                    // refer to call targets or conditional branch targets
                    // For these cases we don't tag the PC stores so just return from here.
                    if (target == ret|| target == ccsrc || target == ccdst)
                        return;

                    // From this point we are only interested in stores to @PC
                    if (target != pc)
                        continue;

                    // This is a constant store to @PC. Mark it as inststart and
                    // update lastpc for the block
                    if (auto ci = dyn_cast<ConstantInt>(store->getValueOperand())) {
                        store->setMetadata("inststart", md);

                        unsigned pc = ci->getZExtValue();
                        if (MDNode *mdlast = binrec::getBlockMeta(&f, "lastpc")) {
                            unsigned curlastpc =
                                cast<ConstantInt>(dyn_cast<ValueAsMetadata>(mdlast->getOperand(0))->getValue())
                                    ->getZExtValue();
                            if (curlastpc >= pc)
                                continue;
                        }
                        binrec::setBlockMeta(&f, "lastpc", pc);
                    }
                }
            }
        }
    }

    void tag_pc(Module &m) {
        GlobalVariable *pc = m.getNamedGlobal("PC");
        // Used for for calls
        GlobalVariable *ret = m.getNamedGlobal("return_address");
        // Used for control flow
        GlobalVariable *ccdst = m.getNamedGlobal("cc_dst");
        GlobalVariable *ccsrc = m.getNamedGlobal("cc_src");

        MDNode *md = MDNode::get(m.getContext(), NULL);

        for (auto &f : m) {
            if (!binrec::is_lifted_function(f))
               continue;
            tag_pc_in_function(f, pc, ret, ccdst, ccsrc, md);
        }
    }
}

// NOLINTNEXTLINE
auto TagInstPcPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    tag_pc(m);

    return PreservedAnalyses::all();
}
