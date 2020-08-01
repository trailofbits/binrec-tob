/*
 * Remove global array 'memory' after replacing all occurences of memory[index]
 * with *index.
 */

#include "llvm/IR/Operator.h"
#include "llvm/IR/Constants.h"

#include "AddMemArray.h"
#include "RemoveMemArray.h"
#include "PassUtils.h"
#include "SectionUtils.h"

using namespace llvm;

char RemoveMemArrayPass::ID = 0;
static RegisterPass<RemoveMemArrayPass> X("remove-mem-array",
        "S2E replace memory array accesses to absolute pointer accesses", false, false);

typedef struct {
    unsigned addr;
    section_meta_t &s;
} param_t;

static bool checkSection(section_meta_t &s, void *param) {
    param_t *p = (param_t*)param;
    if (p->addr >= s.loadBase && p->addr < s.loadBase + s.size) {
        p->s = s;
        return true;
    }
    return false;
}

static inline bool findSection(Module &m, unsigned addr, section_meta_t &s) {
    param_t p = {addr, s};
    return mapToSections(m, checkSection, &p);
}

bool RemoveMemArrayPass::runOnModule(Module &m) {
    std::list<User*> ptrs;
    GlobalVariable *memory = m.getNamedGlobal(MEMNAME);
    assert(memory && "memory variable not found");

    foreach2(user, memory->user_begin(), memory->user_end()) {
        assert(isa<GetElementPtrInst>(*user) || isa<GEPOperator>(*user));
        ptrs.push_back(*user);
    }

    foreach(it, ptrs) {
        User *user = *it;
        Value *idx = user->getOperand(1);

        if (GetElementPtrInst *ptr = dyn_cast<GetElementPtrInst>(user)) {
            ReplaceInstWithInst(ptr, new IntToPtrInst(idx, ptr->getType()));
        } else {
            GEPOperator *op = cast<GEPOperator>(user);
            op->replaceAllUsesWith(ConstantExpr::getIntToPtr(
                        cast<Constant>(idx), op->getType()));
        }
    }

    memory->eraseFromParent();
    return true;
}
