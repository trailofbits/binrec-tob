#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>

#include <s2e/S2E.h>
#include <s2e/Utils.h>

#include <llvm/LLVMContext.h>
#include <llvm/Instructions.h>
#include <llvm/Metadata.h>
#include <llvm/Constants.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include "Export.h"
#include "ModuleSelector.h"

#define WRITE_LLVM_SRC true
#define BINARY_SYMLINK_NAME "binary"
#define BINARY_PATH_ENVNAME "S2E_BINARY"

// (I'm so sorry for writing this code)
// dirty hack to generate unmangled symbols for TCG internals, used by
// forceCodeGen() below
namespace clink {
extern "C" {
#undef _EXEC_ALL_H_
#include <exec-all.h>
#include <tcg/tcg.h>
#include <tcg/tcg-llvm.h>
extern CPUArchState *env;
}
}

namespace s2e {
namespace plugins {

using namespace llvm;

#ifdef TARGET_I386

Export::Export(S2E* s2e) : Plugin(s2e), m_module(NULL), m_exportCounter(0),
    m_regenerateBlocks(true), m_exportInterval(0)
{
}

Export::~Export()
{
    saveLLVMModule(false);
}

static inline bool fileExists(const std::string &name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

void Export::initializeModule(const ModuleDescriptor &module,
        const ConfigFile::string_list &baseDirs)
{
    m_moduleDesc = module;
    const char *path = getenv(BINARY_PATH_ENVNAME);

    if (!path) {
        std::string findPath("");

        foreach2(it, baseDirs.begin(), baseDirs.end()) {
            const std::string &baseDir = *it;

            if (fileExists(baseDir + "/" + module.Name)) {
                findPath = baseDir + "/" + module.Name;
                break;
            }
        }

        assert(findPath.size() && "binary path not found in environment or base dirs");
        path = findPath.c_str();
    }

    // get full path to binary
    char fullProgramPath[PATH_MAX];
    assert(realpath(path, fullProgramPath));

    // create symlink to binary in s2e output dir
    symlink(fullProgramPath, s2e()->getOutputFilename(BINARY_SYMLINK_NAME).c_str());
}

bool Export::exportBB(S2EExecutionState *state, uint64_t pc)
{
    bb_count_t::iterator it = m_bbCounts.find(pc);
    unsigned npassed = it == m_bbCounts.end() ? 0 : it->second;
    Function *f = NULL;

    // only export a block twice (the second time, check for differences and
    // use the second version)
    if (npassed == 0) {
        f = forceCodeGen(state);

        // regenerating BBS breaks symbex, so don't regen and assume the
        // generated block is correct (should be since it is evaluated)
        m_bbCounts[pc] = m_regenerateBlocks ? 1 : 2;
        m_bbFinalized[pc] = false;
    //} else if (npassed == 1 && m_regenerateBlocks) {
    } else if (m_regenerateBlocks && !m_bbFinalized[pc]) {
        f = regenCode(state, m_bbFuns[pc]);
        m_bbCounts[pc] += 1;
    }

    if (!f)
        return false;

    // clone LLVM funcion from translation block
    m_bbFuns[pc] = f;

    if (!m_module)
        m_module = f->getParent();
    else
        assert(m_module == f->getParent() && "LLVM basic blocks saved to different modules");

    if (m_exportInterval && ++m_exportCounter % m_exportInterval == 0)
        saveLLVMModule(true);

    return true;
}

std::string Export::getExportFilename(bool intermediate)
{
    return "captured";
}

void Export::saveLLVMModule(bool intermediate)
{
    std::string dir = s2e()->getOutputFilename("/");
    std::string error;

    s2e()->getMessagesStream() << "Saving LLVM module... ";

    if (!m_module) {
        s2e()->getWarningsStream() << "error: module is uninitialized.  \
            Many errors may occur here";
        return;
    }

    std::string basename = getExportFilename(intermediate);

    raw_fd_ostream bitcodeOstream((dir + basename + ".bc").c_str(), error, 0);
    WriteBitcodeToFile(m_module, bitcodeOstream);
    bitcodeOstream.close();

#if WRITE_LLVM_SRC
    if (!intermediate) {
        raw_fd_ostream llvmOstream((dir + basename + ".ll").c_str(), error, 0);
        llvmOstream << *m_module;
        llvmOstream.close();
    }
#endif

    s2e()->getMessagesStream() << "saving successor lists... ";

    raw_fd_ostream succsOstream(s2e()->getOutputFilename("succs.dat").c_str(), error, 0);
    foreach2(it, m_succs.begin(), m_succs.end()) {
        const uint64_t key = *it;
        succsOstream.write((const char*)&key, sizeof (uint64_t));
    }
    succsOstream.close();

    s2e()->getMessagesStream() << "done\n";
}

static uint64_t getFirstStoredPc(Function *f) {
    assert(!f->empty());
    BasicBlock &bb = f->getEntryBlock();

    foreach2(inst, bb.begin(), bb.end()) {
        StoreInst *store = dyn_cast<StoreInst>(inst);
        if (!store) continue;

        ConstantInt *pcVal = dyn_cast<ConstantInt>(store->getValueOperand());
        if (!pcVal || !pcVal->getType()->isIntegerTy(32)) continue;

        IntToPtrInst *envCast = cast<IntToPtrInst>(store->getPointerOperand());
        assert(envCast->getSrcTy()->isIntegerTy(64));
        assert(cast<PointerType>(envCast->getDestTy())->getElementType()->isIntegerTy(32));

        BinaryOperator *add = cast<BinaryOperator>(envCast->getOperand(0));
        assert(add->getOpcode() == Instruction::Add);
        assert(add->getOperand(0)->getName().startswith("env_v"));
        assert(cast<ConstantInt>(add->getOperand(1))->getZExtValue() == 48);

        return pcVal->getZExtValue();
    }

    return 0;
}

Function *Export::forceCodeGen(S2EExecutionState *state)
{
    TranslationBlock *tb = state->getTb();
    if (tb->llvm_function == NULL) {
        clink::cpu_gen_llvm(clink::env, (struct clink::TranslationBlock*)tb);
        //clink::tcg_llvm_gen_code(clink::tcg_llvm_ctx, &clink::tcg_ctx, (struct clink::TranslationBlock*)tb);
        assert(tb->llvm_function && "no llvm translation block found");

        // check to make sure that the generated code is that emulates the
        // correct PC
        uint64_t pcExpected = state->getPc();
        uint64_t pcFound = getFirstStoredPc(tb->llvm_function);

        if (pcFound != pcExpected) {
            s2e()->getWarningsStream() << "LLVM block for pc " <<
                hexval(pcExpected) << " stores pc " << hexval(pcFound) << ":\n";
            tb->llvm_function->dump();
            assert(false);
        }

        // FIXME: enable this?
        // clean up again to avoid a crash in S2E's cleanup
        // clearLLVMFunction(tb);
    }

    Function *f = tb->llvm_function;
    // clearLLVMFunction(tb);

    return f;
}

static BasicBlock::iterator skipAllocas(BasicBlock::iterator it) {
    while (isa<AllocaInst>(it))
        it++;
    return it;
}


bool Export::isFuncCallAndValid(Instruction *I) {
    if (CallInst *call = dyn_cast<CallInst>(I)) {
        Function *f = call->getCalledFunction();
        if (f && f->hasName() && f->getName() == "helper_raise_exception") {
            return false;
        }
    }
    return true;
}

void Export::evaluateFunctions(Function *aa, Function *bb/*old Func*/, bool *aIsValid, bool *bIsValid) {
    int bbCounter = 0;
    for (Function::iterator blocka = aa->begin(), blockb = bb->begin();
            blocka != aa->end(); blocka++, blockb++) {
        bbCounter++;
        s2e()->getDebugStream() << "-------------BasicBlock " << bbCounter << " --------------\n";
        s2e()->getDebugStream() << "Func new:: bb size: " << blocka->size() << "\n";
        s2e()->getDebugStream() << "Func old:: bb size: " << blockb->size() << "\n";
        s2e()->getDebugStream() << "-------------->\n";
        //s2e()->getDebugStream().flush();
        for (BasicBlock::iterator insta = skipAllocas(blocka->begin()),
           instb = skipAllocas(blockb->begin());
              insta != blocka->end() /*&& instb != blockb->end()*/; insta++, instb++) {
            if (!isFuncCallAndValid(&*insta)) {
                *aIsValid = false;
                s2e()->getDebugStream() << "Func new:: bb " << bbCounter << " has exception call\n";
                s2e()->getDebugStream().flush();
            }
            if (!isFuncCallAndValid(&*instb)) {
                *bIsValid = false;
                s2e()->getDebugStream() << "Func old:: bb " << bbCounter << " has exception call\n";
                s2e()->getDebugStream().flush();
            }

        }


     //   for (BasicBlock::iterator insta = skipAllocas(blocka->begin());
     //           insta != blocka->end(); insta++) {
           /* if (!isFuncCallAndValid(&*insta)) {
                *aIsValid = false;
                s2e()->getDebugStream() << "Func new:: bb " << bbCounter << " has exception call\n";
                s2e()->getDebugStream().flush();
            }*/
     //   }
     //   for (BasicBlock::iterator instb = skipAllocas(blockb->begin());
     //           instb != blockb->end(); instb++) {
            /*if (!isFuncCallAndValid(&*instb)) {
                *bIsValid = false;
                s2e()->getDebugStream() << "Func old:: bb " << bbCounter << " has exception call\n";
                s2e()->getDebugStream().flush();
            }*/
    //    }
    }
}


static BasicBlock::iterator skipInst(BasicBlock::iterator it, int size) {
    if (size < 0)
        return it;
    while (size) {
        it++;
        --size;
    }
    return it;
}
static Function::iterator skipBB(Function::iterator it, int size) {
    if (size < 0)
        return it;
    while (size) {
        it++;
        --size;
    }
    return it;
}


bool Export::areBBsEqual(Function *a, Function *b/*old Func*/, bool *aIsValid, bool *bIsValid) {
    /*int aSumInst = 0;
    int bSumInst = 0;

    for (Function::iterator blocka = a->begin(), blockb = b->begin();
            blocka != a->end() && blockb != b->end(); blocka++, blockb++) {
        BasicBlock::iterator insta = skipAllocas(blocka->begin());
        BasicBlock::iterator instb = skipAllocas(blockb->begin());
        aSumInst += std::distance(insta, blocka->end());
        bSumInst += std::distance(instb, blockb->end());
        for ( ; insta != blocka->end() && instb != blockb->end(); insta++, instb++) {
            if (!isFuncCallAndValid(&*insta)) {
                *aIsValid = false;
                s2e()->getDebugStream() << "Func new:: bb has exception call\n";
            }
            if (!isFuncCallAndValid(&*instb)) {
                *bIsValid = false;
                s2e()->getDebugStream() << "Func old:: bb has exception call\n";
            }

        }
    }*/

    int aSumInst = 0;
    int bSumInst = 0;
    for (Function::iterator blocka = skipBB(a->begin(), a->size() - 1);
            blocka != a->end(); blocka++) {
        //BasicBlock::iterator insta = blocka->begin();
        BasicBlock::iterator insta = skipAllocas(blocka->begin());
        int dist = std::distance(insta, blocka->end());
        aSumInst += dist;
        //exception call is always the second inst from at the end of basic block
        for (insta = skipInst(insta, dist - 3); insta != blocka->end(); insta++) {
            //s2e()->getDebugStream() << "A: " << *insta;
            if (!isFuncCallAndValid(&*insta)) {
                *aIsValid = false;
                s2e()->getDebugStream() << "Func new has exception call\n";
            }
        }
    }
    for (Function::iterator blockb = skipBB(b->begin(), b->size() - 1);
            blockb != b->end(); blockb++) {
        //BasicBlock::iterator instb = blockb->begin();
        BasicBlock::iterator instb = skipAllocas(blockb->begin());
        int dist = std::distance(instb, blockb->end());
        bSumInst += dist;
        //s2e()->getMessagesStream() << "aSumInst:" << aSumInst << " bSumInst:" << bSumInst << "\n";

        //exception call is always the second inst from at the end of basic block
        for (instb = skipInst(instb, dist - 3); instb != blockb->end(); instb++) {
            //s2e()->getDebugStream() << "B: " << *instb;
            if (!isFuncCallAndValid(&*instb)) {
                *bIsValid = false;
                s2e()->getDebugStream() << "Func old has exception call\n";
            }
        }
    }

    int diff = aSumInst - bSumInst;
    if (*aIsValid && *bIsValid && diff != 0){
        s2e()->getDebugStream() << "Exceptional Case: Investigate\n";
        return false;
    }
    for (Function::iterator blockb = skipBB(b->begin(), b->size() - 1);
            blockb != b->end(); blockb++) {
        //BasicBlock::iterator instb = blockb->begin();
        BasicBlock::iterator instb = skipAllocas(blockb->begin());
        int dist = std::distance(instb, blockb->end());
        bSumInst += dist;
        //s2e()->getMessagesStream() << "aSumInst:" << aSumInst << " bSumInst:" << bSumInst << "\n";

        //exception call is always the second inst from at the end of basic block
        for (instb = skipInst(instb, dist - 3); instb != blockb->end(); instb++) {
            //s2e()->getDebugStream() << "B: " << *instb;
            if (!isFuncCallAndValid(&*instb)) {
                *bIsValid = false;
                s2e()->getDebugStream() << "Func old has exception call\n";
            }
        }
    }


    return true;
}


void Export::clearLLVMFunction(TranslationBlock *tb) {
    clink::tcg_llvm_tb_alloc((struct clink::TranslationBlock*)tb);
    s2e_set_tb_function(s2e(), tb);
}

Function *Export::regenCode(S2EExecutionState *state, Function *old)
{
    TranslationBlock *tb = state->getTb();

    if (tb->llvm_function)
        clearLLVMFunction(tb);

    clink::cpu_gen_llvm(clink::env, (struct clink::TranslationBlock*)tb);

    Function *newF = tb->llvm_function;

    bool oldIsValid = true;
    bool newIsValid = true;
    bool equal = areBBsEqual(newF, old, &newIsValid, &oldIsValid);

    if (oldIsValid && newIsValid && equal) {
        //s2e()->getDebugStream() << "Finalized Function: " << "PC= " << hexval(state->getPc()) << " #ofBB= " <<  m_bbCounts[state->getPc()] << "\n";
        m_bbFinalized[state->getPc()] = true;
        clearLLVMFunction(tb);
        return NULL;
    }else if (oldIsValid && !newIsValid) {
        s2e()->getDebugStream() << "Old function is valid but not the new one\n";
        s2e()->getDebugStream() << "function size :: " << "new: " <<
            newF->size() << " vs " << "old: "  << old->size() << "\n";
        s2e()->getDebugStream() << *newF << *old;
        clearLLVMFunction(tb);
        return NULL;
    }else if (!oldIsValid && newIsValid) {
        s2e()->getDebugStream() << "Old function is not valid but the new one is valid\n";
    }else if (!oldIsValid && !newIsValid) {
        s2e()->getDebugStream() << "Both functions are not valid\n";
    }

    s2e()->getDebugStream() << "function size :: " << "new: " <<
       newF->size() << " vs " << "old: " << old->size() << "\n";

    s2e()->getMessagesStream() << "[Export] regenerated BB for pc " <<
       hexval(state->getPc()) << "\n";

    s2e()->getDebugStream() << *newF << *old;

    old->eraseFromParent();

    return tb->llvm_function;

    // FIXME: enable this?
    // Function *f = tb->llvm_function;
    // clearLLVMFunction(tb);
    // return f;
}

Function *Export::getBB(uint64_t pc)
{
    bb_map_t::iterator it = m_bbFuns.find(pc);
    return it == m_bbFuns.end() ? NULL : it->second;
}

bool Export::addSuccessor(uint64_t predPc, uint64_t pc)
{
    if (!predPc || !getBB(pc))
        return false;

    m_succs.insert((predPc << 32) | (pc & 0xffffffff));
    return true;
}

Instruction *Export::getMetadataInst(uint64_t pc)
{
    if (m_bbFuns.find(pc) == m_bbFuns.end())
        return NULL;

    return m_bbFuns[pc]->getEntryBlock().getTerminator();
}

void Export::stopRegeneratingBlocks() {
    m_regenerateBlocks = false;
    s2e()->getMessagesStream() << "stopped regenerating exported blocks\n";
}

#endif // TARGET_I386

} // namespace plugins
} // namespace s2e
