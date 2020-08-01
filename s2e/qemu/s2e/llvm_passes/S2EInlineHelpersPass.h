#include "llvm/Pass.h"

#include <list>

namespace llvm {
	struct CallInst;
	struct Function;
}

  struct S2EInlineHelpersPass : public llvm::FunctionPass {
    static char ID;
    S2EInlineHelpersPass();

    virtual bool runOnFunction(llvm::Function& f);
  private:
    bool shouldInlineCall(llvm::CallInst* call);

  };
