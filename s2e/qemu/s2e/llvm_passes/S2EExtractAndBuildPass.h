#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include <vector>
#include <map>
#include <list>

  struct S2EExtractAndBuildPass : public llvm::FunctionPass {
    typedef std::map<llvm::BasicBlock*, unsigned long> targetmap_t;
    targetmap_t targetMap;
    static char ID;
    S2EExtractAndBuildPass() : llvm::FunctionPass(ID) { initialized = false; }

    virtual bool runOnFunction(llvm::Function &F);
    void setBlockAddr(unsigned long startAddr, unsigned long endAddr) { currBlockAddr = startAddr; nextBlockAddr = endAddr; };
    bool hasImplicitTarget() {return implicitTarget;}
    llvm::Module* getBuiltModule() { return M; }
    size_t getFunctionCount() { return functions.size(); }
    llvm::BasicBlock* getBasicBlockByAddr(uint64_t addr);

  private:
    bool initialized;
    llvm::Module *M;
    unsigned long currBlockAddr;
    unsigned long nextBlockAddr;
    bool implicitTarget;
    typedef std::map<std::string, llvm::BasicBlock*> bbmap_t;
    bbmap_t basicBlocks;
    typedef std::map<std::string, llvm::Function*> fmap_t;
    fmap_t functions;
    llvm::FunctionType* genericFunctionType;
    void initialize(llvm::Function &F);
    void processPCUse(llvm::Value *pcUse);
    bool searchNextBasicBlockAddr(llvm::Function &F);
    llvm::Function* findParentFunction();
    void copyGlobalReferences(llvm::Function &F, llvm::ValueToValueMapTy &VMap);
    void createDirectEdge(llvm::BasicBlock *block, unsigned long target);
    void createIndirectEdge(llvm::BasicBlock *block);
    void createCallEdge(llvm::BasicBlock *block, unsigned long target, bool isTailCall = false);
    void createIndirectCallEdge(llvm::BasicBlock *block);
    llvm::BasicBlock* getBasicBlock(std::string name);
    llvm::BasicBlock* getOrCreateBasicBlock(llvm::Function *parent, std::string name);
    llvm::BasicBlock* getOrCreateTransBasicBlock(llvm::Function *parent, unsigned long addr);
    llvm::Function* getFunction(std::string name);
    llvm::Function* getOrCreateFunction(std::string name);
    void moveBasicBlockToNewFunction(llvm::BasicBlock *block, llvm::Function *parent);
    void moveBasicBlocksToNewFunction(llvm::BasicBlock *block, llvm::Function *parent);
    void relinkOldTransBasicBlock(llvm::Function *newF, std::string addr, std::list<llvm::Instruction *> &terminatorList);
    bool validateMove(llvm::BasicBlock *block);
  };
