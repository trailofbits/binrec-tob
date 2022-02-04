#include "tag_inst_pc.hpp"
#include "ir/selectors.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include <algorithm>

using namespace binrec;
using namespace llvm;


namespace {
    void tag_pc_in_function(Function &f, GlobalVariable *pc, GlobalVariable *ret, MDNode *md)
    {
        // We are only interested in the first blocks (before conditional branches). Other blocks
        // will be handled separately.
        auto bb = &f.getEntryBlock();
        while (bb) {
            for (auto &ins : *bb) {
                if (auto store = dyn_cast<StoreInst>(&ins)) {
                    auto target = store->getPointerOperand();
                    // NOTE (hbrodin): If we store to the return address the next PC store is the
                    // callee, which is not of concern for this block so end here.
                    if (target == ret)
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
                                cast<ConstantInt>(
                                    dyn_cast<ValueAsMetadata>(mdlast->getOperand(0))->getValue())
                                    ->getZExtValue();
                            if (curlastpc >= pc)
                                continue;
                        }
                        binrec::setBlockMeta(&f, "lastpc", pc);
                    }
                }
            }

            auto term = bb->getTerminator();
            if (auto br = dyn_cast<BranchInst>(term)) {
                if (br->isUnconditional()) {
                    bb = dyn_cast<BasicBlock>(br->getOperand(0));
                    continue;
                }
            }
            bb = nullptr;
        }
    }

    void tag_pc(Module &m)
    {
        GlobalVariable *pc = m.getNamedGlobal("PC");
        // Used for for calls
        GlobalVariable *ret = m.getNamedGlobal("return_address");

        MDNode *md = MDNode::get(m.getContext(), NULL);

        for (auto &f : m) {
            if (!binrec::is_lifted_function(f))
                continue;
            tag_pc_in_function(f, pc, ret, md);
        }
    }
} // namespace

// NOLINTNEXTLINE
auto TagInstPcPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    tag_pc(m);

    return PreservedAnalyses::all();
}
