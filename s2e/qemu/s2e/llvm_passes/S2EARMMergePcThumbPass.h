#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"

#include "llvm/Constants.h"

#include "s2e/S2EExecutionState.h"

#include <list>
#include <map>



  struct S2EArmMergePcThumbPass : public llvm::FunctionPass {
    static char ID;

    enum InstructionSet {
    	  ARM,
    	  THUMB,
    	  UNKNOWN
    };
    S2EArmMergePcThumbPass() : llvm::FunctionPass(ID) {}

    virtual bool runOnFunction(llvm::Function &f);
    InstructionSet getInstructionSet(uint64_t pc);
  private:
//    llvm::Module *M;
    std::map<uint64_t, InstructionSet> m_instructionSet;
  };
