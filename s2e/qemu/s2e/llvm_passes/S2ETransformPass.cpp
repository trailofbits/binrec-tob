#include "S2ETransformPass.h"
#include "S2EPassUtils.h"

#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

char S2ETransformPass::ID = 0;
static RegisterPass<S2ETransformPass> X("s2etransform", "S2E Transformation Pass", false, false);

bool S2ETransformPass::runOnFunction(Function &F) {
  M = F.getParent();
  if (M == NULL) {
    // FIXME
    errs() << "Module is NULL\n";
    return false;
  }
  Argument *arg0 = F.getArgumentList().begin();
  bool recursiveSuccess = true;
  for (Value::use_iterator iub = arg0->use_begin(), iue = arg0->use_end(); iub != iue; ++iub) {
      recursiveSuccess &= processArgUse(*iub);
  }
  Value *globalEnv = M->getOrInsertGlobal("CPUSTATE", arg0->getType());
  BasicBlock &entry = F.getEntryBlock();
  LoadInst *loadEnv = new LoadInst(globalEnv, "", entry.getFirstInsertionPt());
  arg0->replaceAllUsesWith(loadEnv);
  cleanup();
  if (recursiveSuccess)
    loadEnv->eraseFromParent();
  return false;
}

bool S2ETransformPass::processArgUse(Value *argUse) {
  bool recursiveSuccess = true;
  Instruction *useInstruction = dyn_cast<Instruction>(argUse);
  if (!useInstruction) {
    errs() << "Undefined use of argument: " << *argUse << "\n";
    return false;
  }

  // Cast instructions just copy value
  // Propagate data-flow analysis
  if (useInstruction->isCast()) {
    for (Value::use_iterator iub = useInstruction->use_begin(), iue = useInstruction->use_end();
        iub != iue; ++iub) {
      recursiveSuccess &= processArgUse(*iub);
    }
    if (recursiveSuccess)
      eraseList.push_back(useInstruction);
    return recursiveSuccess;
  }
  // GetElementPtr instructions can copy value
  // Propagate data-flow analysis with new offset
  GetElementPtrInst *GEPInst = dyn_cast<GetElementPtrInst>(useInstruction);
  if (GEPInst) {
    ConstantInt *constOffset = dyn_cast<ConstantInt>(*(GEPInst->idx_begin()));
    if (GEPInst->getNumIndices() != 1 || !constOffset || constOffset->getSExtValue() != 0) {
      errs() << "Unsupported GEP for argument pointer: " << *useInstruction << "\n";
      return false;
    }
    for (Value::use_iterator iub = useInstruction->use_begin(), iue = useInstruction->use_end();
        iub != iue; ++iub) {
      recursiveSuccess &= processArgUse(*iub);
    }
    if (recursiveSuccess)
      eraseList.push_back(useInstruction);
    return recursiveSuccess;
  }
  // Load instruction uses argument pointer
  // Sink for data-flow analysis
  LoadInst *loadInst = dyn_cast<LoadInst>(useInstruction);
  if (loadInst) {
    for (Value::use_iterator iub = useInstruction->use_begin(), iue = useInstruction->use_end();
        iub != iue; ++iub) {
      recursiveSuccess &= processEnvUse(useInstruction, *iub, 0);
    }
    if (recursiveSuccess)
      eraseList.push_back(useInstruction);
    return recursiveSuccess;
  }
  // Unsupported instruction
  errs() << "Unsupported instruction: " << *useInstruction << "\n";
  return false;
}

bool S2ETransformPass::processEnvUse(Value *envPointer, Value *envUse, unsigned int currentOffset) {
  bool recursiveSuccess = true;
  Instruction *useInstruction = dyn_cast<Instruction>(envUse);
  if (!useInstruction) {
    errs() << "Undefined use of environment pointer: " << *envUse << "\n";
    return false;
  }

  // Cast instructions just copy value
  // Propagate data-flow analysis
  if (useInstruction->isCast()) {
    for (Value::use_iterator iub = useInstruction->use_begin(), iue = useInstruction->use_end();
        iub != iue; ++iub) {
      recursiveSuccess &= processEnvUse(envUse, *iub, currentOffset);
    }
    if (recursiveSuccess)
      eraseList.push_back(useInstruction);
    return recursiveSuccess;
  }
  // Add instructions increase offset
  // Propagate data-flow analysis with new offset
  if (useInstruction->isBinaryOp() && useInstruction->getOpcode() == Instruction::Add) {
    ConstantInt *constOffset = dyn_cast<ConstantInt>(useInstruction->getOperand(1));
    if (!constOffset) {
      errs() << "Increase by non-constant offset for environment pointer: " << *useInstruction << "\n";
      return false;
    }
    for (Value::use_iterator iub = useInstruction->use_begin(), iue = useInstruction->use_end();
        iub != iue; ++iub) {
      recursiveSuccess &= processEnvUse(envUse, *iub, currentOffset + constOffset->getSExtValue());
    }
    if (recursiveSuccess)
      eraseList.push_back(useInstruction);
    return recursiveSuccess;
  }
  // GetElementPtr instructions increase offset
  // Propagate data-flow analysis with new offset
  GetElementPtrInst *GEPInst = dyn_cast<GetElementPtrInst>(useInstruction);
  if (GEPInst) {
    ConstantInt *constOffset = dyn_cast<ConstantInt>(*(GEPInst->idx_begin()));
    if (GEPInst->getNumIndices() != 1 || !constOffset) {
      errs() << "Increase by non-constant offset for environment pointer: " << *useInstruction << "\n";
      return false;
    }
    Type* type = GEPInst->getPointerOperandType()->getPointerElementType();
    unsigned int sizeInBytes = (type->getPrimitiveSizeInBits() + 7) / 8;
    for (Value::use_iterator iub = useInstruction->use_begin(), iue = useInstruction->use_end();
        iub != iue; ++iub) {
      recursiveSuccess &= processEnvUse(envUse, *iub, currentOffset + constOffset->getSExtValue() * sizeInBytes);
    }
    if (recursiveSuccess)
      eraseList.push_back(useInstruction);
    return recursiveSuccess;
  }
  // Load instruction uses environment pointer
  // Sink for data-flow analysis
  LoadInst *loadInst = dyn_cast<LoadInst>(useInstruction);
  if (loadInst) {
    Value *global = M->getOrInsertGlobal(getGlobalName(currentOffset), loadInst->getType());
    LoadInst *newLoad = new LoadInst (global, "", loadInst);
    replaceMap.insert(std::make_pair(loadInst, newLoad));
    eraseList.push_back(loadInst);
    return true;
  }
  // Store instruction uses environment pointer
  // Sink for data-flow analysis
  StoreInst *storeInst = dyn_cast<StoreInst>(useInstruction);
  if (storeInst) {
    if (storeInst->getPointerOperand() != envPointer) {
      errs() << "Environment pointer stored in memory: " << *useInstruction << "\n";
      return false;
    }
    Value *global = M->getOrInsertGlobal(getGlobalName(currentOffset), storeInst->getValueOperand()->getType());
    StoreInst *newStore = new StoreInst (storeInst->getValueOperand(), global, storeInst);
    replaceMap.insert(std::make_pair(storeInst, newStore));
    eraseList.push_back(storeInst);
    return true;
  }
  // Call instruction uses environment pointer
  // Sink for data-flow analysis
  CallInst *callInst = dyn_cast<CallInst>(useInstruction);
  if (callInst) {
    return false;
  }
  // Unsupported instruction
  errs() << "Unsupported instruction: " << *useInstruction << "\n";
  return false;
}

void S2ETransformPass::cleanup() {
  while(replaceMap.begin() != replaceMap.end()) {
    replaceMap.begin()->first->replaceAllUsesWith(replaceMap.begin()->second);
    replaceMap.erase(replaceMap.begin()->first);
  }
  while(eraseList.begin() != eraseList.end()) {
    (*eraseList.begin())->eraseFromParent();
    eraseList.pop_front();
  }
}

void S2ETransformPass::setRegisterName(unsigned int offset, std::string name) {
  nameMap.insert(make_pair(offset, name));
}

void S2ETransformPass::initRegisterNamesX86() {
  setRegisterName(0 * 4, "R_EAX");
  setRegisterName(1 * 4, "R_ECX");
  setRegisterName(2 * 4, "R_EDX");
  setRegisterName(3 * 4, "R_EBX");
  setRegisterName(4 * 4, "R_ESP");
  setRegisterName(5 * 4, "R_EBP");
  setRegisterName(6 * 4, "R_ESI");
  setRegisterName(7 * 4, "R_EDI");
  setRegisterName(12 * 4, "PC");
}

void S2ETransformPass::initRegisterNamesARM() {
  for(int i = 0; i < 13; ++i) {
    setRegisterName((33 + i) * 4, "R" + intToString(i));
  }
  setRegisterName((33 + 13) * 4, "SP");
  setRegisterName((33 + 14) * 4, "LR");
  setRegisterName((33 + 15) * 4, "PC");
  setRegisterName(204, "thumb");
  setRegisterName(29 * 4, "CF");
  setRegisterName(30 * 4, "VF");
  setRegisterName(31 * 4, "NF");
  setRegisterName(32 * 4, "ZF");
}

std::string S2ETransformPass::getGlobalName(unsigned int offset) {
  if (nameMap.count(offset))
    return nameMap[offset];
  return "GLOBAL_" + intToString(offset);
}
