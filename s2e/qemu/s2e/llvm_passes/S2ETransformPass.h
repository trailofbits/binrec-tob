#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"

#include <list>
#include <map>

  struct S2ETransformPass : public llvm::FunctionPass {
    static char ID;
    S2ETransformPass() : llvm::FunctionPass(ID) {}

    virtual bool runOnFunction(llvm::Function &F);
    void setRegisterName(unsigned int offset, std::string name);
    void initRegisterNamesX86();
    void initRegisterNamesARM();

  private:
    llvm::Module *M;
    std::list<llvm::Instruction*> eraseList;
    std::map<llvm::Instruction*, llvm::Instruction*> replaceMap;
    std::map<unsigned int, std::string> nameMap;
    bool processArgUse(llvm::Value* argUse);
    bool processEnvUse(llvm::Value *envPointer, llvm::Value* envUse, unsigned int currentOffset);
    void cleanup();
    std::string getGlobalName(unsigned int offset);
  };
