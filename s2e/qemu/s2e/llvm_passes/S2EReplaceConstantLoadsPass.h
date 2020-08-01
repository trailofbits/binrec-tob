#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"

#include "llvm/Constants.h"

#include "s2e/S2EExecutionState.h"

#include <vector>
#include <map>

  struct ConstantMemoryRegion  {
    uint64_t address;
    uint64_t size;

    ConstantMemoryRegion(uint64_t address, uint64_t size) : address(address), size(size) {}
  };

  struct S2EReplaceConstantLoadsPass : public llvm::FunctionPass {
    static char ID;
    S2EReplaceConstantLoadsPass() : llvm::FunctionPass(ID), m_bigEndian(false), m_state(0) {}
    S2EReplaceConstantLoadsPass(s2e::S2EExecutionState* state,
    		std::vector<std::pair< uint64_t, uint64_t> > constantRegions,
    		bool bigEndian = false) : llvm::FunctionPass(ID), m_bigEndian(bigEndian), m_state(state), m_constantRegions(constantRegions) {}

    virtual bool runOnFunction(llvm::Function &f);

  private:
    bool m_bigEndian;
    s2e::S2EExecutionState* m_state;
    std::vector<std::pair< uint64_t, uint64_t> > m_constantRegions;
    llvm::ConstantInt* getConstantValue(uint64_t address, llvm::IntegerType* type);
  };
