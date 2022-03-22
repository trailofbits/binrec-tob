#include "tag_inst_pc.hpp"
#include "ir/selectors.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include <algorithm>

using namespace binrec;
using namespace llvm;


namespace {
    /*
     * Check if the @return_address global is updated prior to the given instruction
     * within the same basic block. This method checks if a "store *, @return_address"
     * occurs prior to the given instruction in the containing basic block.
     *
     * This method is used to filter PC values. We ignore PC values that occur after a
     * store to @return_address because this heuristic is indicative of a function call
     * or return.
     */
    bool follows_store_return_address(Instruction *inst, GlobalVariable *return_address)
    {
        const Instruction *prev = inst->getPrevNonDebugInstruction();
        while (prev) {
            auto store = dyn_cast<StoreInst>(prev);
            if (store && store->getPointerOperand() == return_address) {
                return true;
            }

            prev = prev->getPrevNonDebugInstruction();
        }

        return false;
    }

    void tag_pc(Module &m)
    {
        MDNode *md = MDNode::get(m.getContext(), NULL);

        GlobalVariable *pc = m.getNamedGlobal("PC");
        // Used for for calls
        GlobalVariable *ret = m.getNamedGlobal("return_address");

        // iterate over all uses of the @PC global
        for (Use &use : pc->uses()) {
            // filter to only uses that are store const integer instructions within a
            // lifted function that do not follow a store to @return_address.
            auto store = dyn_cast<StoreInst>(use.getUser());
            if (!store) {
                continue;
            }

            auto pc_value = dyn_cast<ConstantInt>(store->getValueOperand());
            if (!pc_value) {
                continue;
            }

            Function *func = store->getFunction();
            if (!func || !binrec::is_lifted_function(*func)) {
                continue;
            }

            if (follows_store_return_address(store, ret)) {
                continue;
            }

            // this is the start of an instruction
            store->setMetadata("inststart", md);

            // Verify that the next instruction is not a return instruction. If it is,
            // ignore this PC value.
            auto next = store->getNextNonDebugInstruction();
            if (next && isa<ReturnInst>(next)) {
                continue;
            }

            // Get the last PC value for this function.
            MDNode *mdlast = binrec::getBlockMeta(func, "lastpc");
            unsigned current_pc = pc_value->getZExtValue();
            unsigned lastpc = 0;

            if (mdlast) {
                lastpc =
                    cast<ConstantInt>(dyn_cast<ValueAsMetadata>(mdlast->getOperand(0))->getValue())
                        ->getZExtValue();
            }

            if (current_pc > lastpc) {
                // update the lastpc metadata for the function
                binrec::setBlockMeta(func, "lastpc", current_pc);
            }
        }
    }
} // namespace

// NOLINTNEXTLINE
auto TagInstPcPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    tag_pc(m);

    return PreservedAnalyses::all();
}
