#include "simplify_stack_offsets.hpp"
#include "pass_utils.hpp"

using namespace llvm;

char SimplifyStackOffsets::ID = 0;
static RegisterPass<SimplifyStackOffsets>
    x("simplify-stack-offsets",
      "S2E Move additions with bitcast stack pointer to getelementptr offset",
      false,
      false);

// example:
// store i32 134515227, i32* inttoptr (i32 add (i32 ptrtoint (i32* getelementptr
// inbounds ([4194304 x i32]* @stack, i32 0, i32 4194300) to i32), i32 -48) to
// i32*), align 4 becomes: store i32 134515227, i32* getelementptr inbounds
// ([4194304 x i32]* @stack, i32 0, i32 4194288), align 4

auto SimplifyStackOffsets::runOnModule(Module &m) -> bool
{
    GlobalVariable *stack = m.getNamedGlobal("stack");
    assert(stack && "stack variable not found");
    auto *stackTy = cast<ArrayType>(stack->getType()->getElementType());
    unsigned wordWidth = cast<IntegerType>(stackTy->getElementType())->getBitWidth() / 8;

    std::list<AddOperator *> replaceList;

    for (User *use : stack->users()) {
        if (auto *gep = dyn_cast<GEPOperator>(use)) {
            for (User *use : gep->users()) {
                if (auto *expr = dyn_cast<ConstantExpr>(use)) {
                    if (expr->getOpcode() == Instruction::PtrToInt) {
                        for (User *use : expr->users()) {
                            if (auto *add = dyn_cast<AddOperator>(use)) {
                                replaceList.push_back(add);
                            }
                        }
                    }
                }
            }
        }
    }

    for (AddOperator *add : replaceList) {
        int relativeOffset = cast<ConstantInt>(add->getOperand(1))->getSExtValue();

        if (relativeOffset % wordWidth != 0) {
            errs() << "non-word-aligned stack offset " << relativeOffset << ": " << *add << "\n";
            exit(1);
        }

        auto *gep = cast<GEPOperator>(cast<ConstantExpr>(add->getOperand(0))->getOperand(0));
        assert(cast<ConstantInt>(gep->getOperand(1))->getZExtValue() == 0);

        unsigned oldOffset = cast<ConstantInt>(gep->getOperand(2))->getZExtValue();
        unsigned newOffset = oldOffset + relativeOffset / wordWidth;

        IntegerType *i32Ty = Type::getInt32Ty(m.getContext());
        Constant *idx[] = {ConstantInt::get(i32Ty, 0), ConstantInt::get(i32Ty, newOffset)};
        Constant *replacement = ConstantExpr::getPtrToInt(
            // TODO(aaltinay): Without looking at the semantics, I fixed it by
            // nullptr. Check if nullptr what we really need to pass as Type*. This
            // change introduced in commit: 19443c1bcb863ba186abfe0bda3a1603488d17f7
            ConstantExpr::getGetElementPtr(
                nullptr,
                cast<Constant>(gep->getPointerOperand()),
                idx,
                gep->isInBounds()),
            add->getType());

        add->replaceAllUsesWith(replacement);
    }

    for (User *use : stack->users()) {
        User *user = cast<User>(use);
        if (!isa<GEPOperator>(user))
            errs() << "no gep operator: " << *user << "\n";
    }

    return true;
}
