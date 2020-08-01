#include <stdio.h>
#include "BPatch.h"
#include "BPatch_addressSpace.h"
#include "BPatch_process.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_function.h"
#include "BPatch_flowGraph.h"

using namespace std;
using namespace Dyninst;
// Create an instance of class BPatch
 BPatch bpatch;
 // Different ways to perform instrumentation
typedef enum {
    create,
    attach,
    open
} accessType_t;

//generic function from sample code, can be smaller if needed
BPatch_addressSpace* startInstrumenting(accessType_t accessType,
 const char* name, int pid) {
    BPatch_addressSpace* handle = NULL;
    switch(accessType) {
        case create:
            handle = bpatch.processCreate(name, argv);
            if (!handle) { fprintf(stderr, "processCreate failed\n");  }
            break;
        case attach:
            handle = bpatch.processAttach(name, pid);
            if (!handle) { fprintf(stderr, "processAttach failed\n");  }
             break;
        case open:
            // Open the binary file and all dependencies
            handle = bpatch.openBinary(name, true);
            if (!handle) { fprintf(stderr, "openBinary failed\n");  }
            break;
    }
    return handle;
}

int binaryAnalysis(BPatch_addressSpace* app) {
    BPatch_image* appImage = app->getImage();
    int insns_access_memory = 0;
    std::vector<BPatch_function*> functions;
    appImage->findFunction("main", functions);
    if (functions.size() == 0) {
        fprintf(stderr, "No function InterestingProcedure\n");
        return insns_access_memory;
    } else if (functions.size() > 1) {
        fprintf(stderr, "More than one InterestingProcedure; using the first one\n");
    }
    BPatch_flowGraph* fg = functions[0]->getCFG();
    std::set<BPatch_basicBlock*> blocks;
    fg->getAllBasicBlocks(blocks);
    for (auto block_iter = blocks.begin(); block_iter != blocks.end(); ++block_iter) {
        BPatch_basicBlock* block = *block_iter;
        std::vector<InstructionAPI::Instruction::Ptr> insns;
        block->getInstructions(insns);
        for (auto insn_iter = insns.begin();insn_iter != insns.end();++insn_iter) {
            InstructionAPI::Instruction::Ptr insn = *insn_iter;
            if (insn->readsMemory() || insn->writesMemory()) { 
                insns_access_memory++;
            }
        }
    }
    return insns_access_memory;
}



//this uses dyninst api, but we need patchapi
//note: bool replaceCode(BPatch_point *point, BPatch_snippet *snippet) This function has been removed; users interested in replacing code should instead use the
//PatchAPI code modification interface described in the PatchAPI manual
//we have to replace, not insert, so the layout of the original binary does not change
int workFunc(BPatch_addressSpace* app, std::vector<uint> patchSites, std::vector<uint> trampLocs) {
//    BPatch_image* appImage = app->getImage();
//    //std::vector<BPatch_function*> functions;
//
//	if (patchSites.size() != trampLocs.size()){
//		fprintf(stderr, "patchSites & trampLocs length differ\n");
//		return -1;
//	}
//	//patchSites to Bpatch points
//    std::vector<BPatch_point *> tempPoints;
//    for (int i =0; i<patchSites.size();i++) {
//        appImage->findPoints(patchSites[i], &tempPoints);
//
//        if (tempPoints.size() == 0) {
//            fprintf(stderr, "Dyninst did not find known address point\n");
//            return -1;
//        }
//        else if (tempPoints.size() > 1) {
//            fprintf(stderr, "Dyninst found overlapping functions at this addr, check if error\n");
//        }
//
//        InstructionAPI::Instruction::Ptr ps1 = tempPoints[0].getInstructionAtPoint();
//        PatchAPI::PatchMgr* mgr = PatchAPI::PatchMgr::create(PatchAPI::convert(app));
//        PatchAPI::PatchBlock *pblock = PatchAPI::convert( tempPoints[0]->getBlock() );
//        PatchAPI::Point* paPoint = mgr->findPoint(PatchAPI::Location::Instruction(pblock,tempPoints[0].getAddress()),None,1);
//        if (!paPoint) {
//            fprintf(stderr, "Dyninst error finding instruction point to replace\n");
//            return -1;
//        }
//
//
//        Instruction::Ptr iptr = pblock->getInsn( (Address) point->getAddress() );

//        const unsigned char *jumpCmdBuffer = "jmp 0xtrampaddr[i]";
//        InstructionAPI::InstructionDecoder decoder1 = InstructionAPI::InstructionDecoder(jumpCmdBuffer,
//                                                                                         strlen(jumpCmdBuffer),
//                                                                                         Arch_x86);
//        InstructionAPI::Instruction::Ptr replaceWith = decoder1.decode();


//    }
//    if (functions.size() == 0) {
//        fprintf(stderr, "No function InterestingProcedure\n");
//        return insns_access_memory;
//    } else if (functions.size() > 1) {
//        fprintf(stderr, "More than one InterestingProcedure; using the first one\n");
//    }
//    BPatch_flowGraph* fg = functions[0]->getCFG();
//    std::set<BPatch_basicBlock*> blocks;
//    fg->getAllBasicBlocks(blocks);
//    for (auto block_iter = blocks.begin(); block_iter != blocks.end(); ++block_iter) {
//        BPatch_basicBlock* block = *block_iter;
//        std::vector<InstructionAPI::Instruction::Ptr> insns;
//        block->getInstructions(insns);
//        for (auto insn_iter = insns.begin();insn_iter != insns.end();++insn_iter) {
//            InstructionAPI::Instruction::Ptr insn = *insn_iter;
//            if (insn->readsMemory() || insn->writesMemory()) {
//        	fprintf(stderr, "called inner if\n");
//                insns_access_memory++;
//            }
//        }
//    }
    return -1;

}

int workFunc2(BPatch_addressSpace* app, std::vector<uint> patchSites, std::vector<uint> trampLocs) {
//    ParseAPI::CodeObject* co =;
//    PatchObject* obj = PatchObject::create(co, code_base);
//    PatchFunction* func = ...;
//    PatchBlock* block = ...;
//    PatchEdge* edge = ...;
//    PatchMgr* mgr = ...;
//    //perhaps todo, specialize address space, snippet, code parse, point make, instrument, then register
//    std::vector<Point*> pts;
//    mgr->findPoints(Scope(),Point::,back_inserter(pts));
//    MySnippet::ptr snippet = MySnippet::create(new MySnippet);
//    Patcher patcher(mgr);
//    for (vector<Point*>::iterator iter = pts.begin(); iter != pts.end(); ++iter) {
//        Point* pt = *iter;
//        patcher.add(PushBackCommand::create(pt, snippet));
//    }
//    patcher.commit();

    return -1;

}



int main(int argc, const char* argv[]) {
    //using generic sample code to do static rewriting
    //expects name of binary to rewrite as arg1
    const char* progName = argv[1];
    const int progPID = nullptr;
    //const char* progArgv[] = {"unused", "-h", NULL};
    const char* progArgv[] = nullptr;
    accessType_t mode = open;
    // Create/attach/open a binary
    BPatch_addressSpace* app = startInstrumenting(mode, progName, progPID, progArgv);
    if (!app) {
        fprintf(stderr, "startInstrumenting failed\n");
        exit(1);
    }

    //read in patch sites and trap locs 

//    int memAccesses = workFunc2(app,pS,tL);

    fprintf(stderr, "Completed Overwriting\n");
}

