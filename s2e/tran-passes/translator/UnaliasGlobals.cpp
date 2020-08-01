#include <llvm/IR/Metadata.h>
#include "AddMemArray.h"
#include "UnaliasGlobals.h"
#include "PassUtils.h"

using namespace llvm;

char UnaliasGlobals::ID = 0;
static RegisterPass<UnaliasGlobals> X("unalias-globals",
        "S2E Add aliasing information to loads of global variables and "
        "@memory array to avoid aliasing between virtual registers and memory",
        false, false);

static MDNode *createDomainAndScopeList(Module &m, StringRef domainName) {
    LLVMContext &ctx = m.getContext();
    MDNode *domain = MDNode::get(ctx, MDString::get(ctx, domainName));
    Metadata *scopeContents[] = {MDString::get(ctx, domainName), domain}; // FIXME: self-reference
    MDNode *scope = MDNode::get(ctx, scopeContents);
    return MDNode::get(ctx, scope);
}

bool UnaliasGlobals::doInitialization(Module &m) {
    // Save memory variable reference for efficient laod/store checking
    memory = m.getNamedGlobal(MEMNAME);
    assert(memory && "no global memory variable was found");

    // Create scopes
    registerScope = createDomainAndScopeList(m, "registers");
    memoryScope = createDomainAndScopeList(m, "memory");

    return true;
}

bool UnaliasGlobals::runOnBasicBlock(BasicBlock &bb) {
    // This assumes that registers are always loaded from / stored to directly
    // instead of through a bitcast, which is probalby true for unoptimized
    // bitcode but maybe notfor optimized

    for (Instruction &i : bb) {
        if (LoadInst *load = dyn_cast<LoadInst>(&i)) {
            GlobalVariable *glob = dyn_cast<GlobalVariable>(load->getPointerOperand());
            if (glob && glob != memory)
                i.setMetadata("alias.scope", registerScope);
            else
                i.setMetadata("alias.scope", memoryScope);
        }
        else if (StoreInst *store = dyn_cast<StoreInst>(&i)) {
            GlobalVariable *glob = dyn_cast<GlobalVariable>(store->getPointerOperand());
            if (glob && glob != memory)
                i.setMetadata("noalias", memoryScope);
            else
                i.setMetadata("noalias", registerScope);
        }
    }

    return true;
}
