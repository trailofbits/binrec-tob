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
        unsigned prev_pc = 0;             // the last stored @PC value
        unsigned max_pc = 0;              // the current maximum @PC value in this function
        bool last_inst_stored_pc = false; // the last instruction updated max_pc

        // load the lastpc from the function, which may be already set
        if (MDNode *mdlast = binrec::getBlockMeta(&f, "lastpc")) {
            unsigned curlastpc =
                cast<ConstantInt>(dyn_cast<ValueAsMetadata>(mdlast->getOperand(0))->getValue())
                    ->getZExtValue();
            if (curlastpc) {
                max_pc = curlastpc;
                prev_pc = curlastpc;
            }
        }

        while (bb) {
            for (auto &ins : *bb) {
                if (auto store = dyn_cast<StoreInst>(&ins)) {
                    auto target = store->getPointerOperand();
                    last_inst_stored_pc = false;

                    // NOTE (hbrodin): If we store to the return address the next PC store is the
                    // callee, which is not of concern for this block so end here.
                    if (target == ret) {
                        assert(max_pc);
                        binrec::setBlockMeta(&f, "lastpc", max_pc);
                        return;
                    }

                    // From this point we are only interested in stores to @PC
                    if (target != pc) {
                        continue;
                    }

                    // This is a constant store to @PC. Mark it as inststart and
                    // update lastpc for the block
                    if (auto ci = dyn_cast<ConstantInt>(store->getValueOperand())) {
                        store->setMetadata("inststart", md);

                        unsigned current_pc = ci->getZExtValue();
                        if (current_pc > max_pc) {
                            // only update the max @PC stored in this function
                            prev_pc = max_pc;
                            max_pc = current_pc;
                            last_inst_stored_pc = true;
                        }
                    }
                } else {
                    if (dyn_cast<ReturnInst>(&ins) && last_inst_stored_pc && prev_pc) {
                        // This is a "ret" instruction. If the previous instruction
                        // was "load @PC ...", then we ignore the last @PC value that
                        // was used, because this PC is most likely a return address
                        // and is not valid.
                        //
                        // See Github issue #90:
                        // https://github.com/trailofbits/binrec-prerelease/issues/90
                        DBG("ignoring last pc, likely a return: " << f.getName() << ": " << max_pc
                                                                  << " -> " << prev_pc);
                        max_pc = prev_pc;
                    }
                    last_inst_stored_pc = false;
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

        assert(max_pc);
        binrec::setBlockMeta(&f, "lastpc", max_pc);
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
