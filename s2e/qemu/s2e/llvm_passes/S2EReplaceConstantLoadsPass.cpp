#include "S2EReplaceConstantLoadsPass.h"
#include "S2EPassUtils.h"

#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/Support/raw_ostream.h"

#include "s2e/S2E.h"

using namespace llvm;

char S2EReplaceConstantLoadsPass::ID = 0;
static RegisterPass<S2EReplaceConstantLoadsPass> X("s2ereplaceconstantloads", "S2E Replace Constant Loads Pass", false, false);

bool S2EReplaceConstantLoadsPass::runOnFunction(Function &f) {
//  M = bb.getParent();
  std::list<llvm::Instruction*> eraseList;
  for (Function::iterator bb = f.begin(); bb != f.end(); bb++)  {
      for (BasicBlock::iterator inst = bb->begin(); inst != bb->end(); inst++) {
        CallInst* callInst = dyn_cast<CallInst>(&*inst);
        if (!callInst)  {
            continue;
        }

        if (callInst->getCalledFunction()->getName() == "__ldl_mmu" && callInst->getNumArgOperands() == 2)  {
          ConstantInt* address = dyn_cast<ConstantInt>(callInst->getArgOperand(0));
          if (!address)  {
            continue;
          }

          ConstantInt* value  = getConstantValue(address->getZExtValue(), llvm::Type::getInt32Ty(bb->getContext()));
          if (value)  {
            callInst->replaceAllUsesWith(value);
            eraseList.push_back(callInst);
          }
        }
        else if (callInst->getCalledFunction()->getName() == "__lds_mmu" && callInst->getNumArgOperands() == 2)  {
          ConstantInt* address = dyn_cast<ConstantInt>(callInst->getArgOperand(0));
          if (!address)  {
            continue;
          }

          ConstantInt* value = getConstantValue(address->getZExtValue(), llvm::Type::getInt16Ty(bb->getContext()));
          if (value)  {
            callInst->replaceAllUsesWith(value);
            eraseList.push_back(callInst);
          }
        }
        else if (callInst->getCalledFunction()->getName() == "__ldb_mmu" && callInst->getNumArgOperands() == 2)  {
          ConstantInt* address = dyn_cast<ConstantInt>(callInst->getArgOperand(0));
          if (!address)  {
            continue;
          }

          ConstantInt* value = getConstantValue(address->getZExtValue(), llvm::Type::getInt8Ty(bb->getContext()));
          if (value)  {
            callInst->replaceAllUsesWith(value);
            eraseList.push_back(callInst);
          }
        }
    }
  }

  for (std::list<llvm::Instruction*>::iterator itr = eraseList.begin(); itr != eraseList.end(); itr++) {
    (*itr)->eraseFromParent();
  }

  return true;
}

ConstantInt* S2EReplaceConstantLoadsPass::getConstantValue(uint64_t address, IntegerType* type)
{
  for (std::vector<std::pair< uint64_t, uint64_t> >::iterator itr = m_constantRegions.begin(); itr != m_constantRegions.end(); itr++)
  {
    if (address >= itr->first && address < itr->first + itr->second)  {
      //TODO: depends on endianness
      uint64_t value = 0;
      uint8_t data[8];
      m_state->readMemoryConcrete(address, data, type->getBitWidth() / 8, S2EExecutionState::VirtualAddress);
      for (unsigned i = 0; i < type->getBitWidth() / 8; i++)  {
    	  if (m_bigEndian)
    	  {
    		  value |= static_cast<uint64_t>(data[i]) << (type->getBitWidth() - 8 * (i + 1));
    	  }
    	  else {
    		  value |= static_cast<uint64_t>(data[i]) << (8 * i);
    	  }
      }

      return ConstantInt::get(type, value, false);
    }
  }

  return 0;
}

