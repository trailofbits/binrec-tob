/* vim: set expandtab ts=4 sw=4: */
/*
 * S2E Selective Symbolic Execution Framework
 *
 * Copyright (c) 2014, EURECOM
 * Copyright (c) 2010, Dependable Systems Laboratory, EPFL
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Dependable Systems Laboratory, EPFL nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE DEPENDABLE SYSTEMS LABORATORY, EPFL BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Currently maintained by:
 *    Jonas Zaddach <zaddach@eurecom.fr>
 *
 *    Vitaly Chipounov <vitaly.chipounov@epfl.ch>
 *    Volodymyr Kuznetsov <vova.kuznetsov@epfl.ch>
 *
 * All contributors are listed in the S2E-AUTHORS file.
 */

/**
 * How this stuff is working:
 * In a first step, we simply want to traverse static jumps and
 */

#include "RecursiveDescentDisassembler.h"
#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>
#include <s2e/S2EExecutor.h>

#include <s2e/cajun/json/reader.h>
#include <s2e/cajun/json/writer.h>

#include <iostream>

#include <llvm/Function.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Transforms/Scalar.h>

#include "s2e/llvm_passes/S2EInlineHelpersPass.h"

using klee::ConstantExpr;
using std::map;

extern "C" int is_valid_code_addr(CPUArchState* env1, target_ulong addr);

namespace s2e {
namespace plugins {

S2E_DEFINE_PLUGIN(RecursiveDescentDisassembler, "Translates the given program to TCG/LLVM by traversing it with recursive-descent semantics", "Disassembler",);

static inline std::string toHexString(uint64_t pc) {
	std::stringstream ss;

	ss << "0x" << std::setw(8) << std::setfill('0') << std::hex << pc;
	return ss.str();
}

void RecursiveDescentDisassembler::initialize()
{
    m_printFunctionBeforeOptimization = s2e()->getConfig()->getBool(
                        getConfigKey() + ".printFunctionBeforeOptimization", false);
    m_printFunctionAfterOptimization = s2e()->getConfig()->getBool(
                        getConfigKey() + ".printFunctionAfterOptimization", false);
    m_verbose = s2e()->getConfig()->getBool(
                        getConfigKey() + ".verbose", false);
    std::string initialEntries = s2e()->getConfig()->getString(
                        getConfigKey() + ".initialEntries", "");
    if (initialEntries != "") {
        std::istream *stream = new std::ifstream(initialEntries.c_str(), std::ios::in | std::ios::binary);
        assert(stream);

        json::Array root;
        json::Reader::Read(root, *stream);
        json::Array::const_iterator entry(root.Begin()),
            entriesEnd(root.End());
        for (; entry != entriesEnd; ++entry) {
            const json::Number &jpc = *entry;
            uint64_t pc = (uint64_t)static_cast<double>(jpc);
            m_bbsTodo.push_back(pc);
        }

        //stream->close();
        /* we should have an array with all the entry points */
    }

    m_initialized = false;

#if defined(TARGET_ARM)
    m_transformPass.setRegisterName(CPU_OFFSET(thumb), "thumb");
    m_transformPass.setRegisterName(CPU_OFFSET(ZF), "ZF");
    m_transformPass.setRegisterName(CPU_OFFSET(CF), "CF");
    m_transformPass.setRegisterName(CPU_OFFSET(VF), "VF");
    m_transformPass.setRegisterName(CPU_OFFSET(NF), "NF");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[15]), "PC");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[14]), "LR");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[13]), "SP");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[12]), "R12");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[11]), "R11");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[10]), "R10");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[9]), "R9");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[8]), "R8");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[7]), "R7");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[6]), "R6");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[5]), "R5");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[4]), "R4");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[3]), "R3");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[2]), "R2");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[1]), "R1");
    m_transformPass.setRegisterName(CPU_OFFSET(regs[0]), "R0");
    m_transformPass.setRegisterName(CPU_OFFSET(s2e_icount), "s2e_icount");
    m_transformPass.setRegisterName(CPU_OFFSET(current_tb), "current_tb");
    m_transformPass.setRegisterName(CPU_OFFSET(s2e_current_tb), "s2e_icount");
#elif defined(TARGET_I386)
    m_transformPass.initRegisterNamesX86();
    m_transformPass.setRegisterName(CPU_OFFSET(cr[0]), "CR0");
    m_transformPass.setRegisterName(CPU_OFFSET(s2e_icount), "s2e_icount");
    m_transformPass.setRegisterName(CPU_OFFSET(current_tb), "current_tb");
    m_transformPass.setRegisterName(CPU_OFFSET(s2e_current_tb), "s2e_icount");
    m_transformPass.setRegisterName(CPU_OFFSET(segs[0].base), "Seg0.base");
    m_transformPass.setRegisterName(CPU_OFFSET(segs[1].base), "Seg1.base");
    m_transformPass.setRegisterName(CPU_OFFSET(segs[2].base), "Seg2.base");
    m_transformPass.setRegisterName(CPU_OFFSET(segs[3].base), "Seg3.base");
    m_transformPass.setRegisterName(CPU_OFFSET(segs[4].base), "Seg4.base");
    m_transformPass.setRegisterName(CPU_OFFSET(segs[5].base), "Seg5.base");
    m_transformPass.setRegisterName(CPU_OFFSET(cc_src), "cc_src");
    m_transformPass.setRegisterName(CPU_OFFSET(cc_dst), "cc_dst");
    m_transformPass.setRegisterName(CPU_OFFSET(cc_tmp), "cc_tmp");
    m_transformPass.setRegisterName(CPU_OFFSET(cc_op), "cc_op");
    m_transformPass.setRegisterName(CPU_OFFSET(df), "df");
#endif


    s2e()->getCorePlugin()->onTranslateBlockStart.connect(
            sigc::mem_fun(*this, &RecursiveDescentDisassembler::slotTranslateBlockStart));
    s2e()->getCorePlugin()->onStateKill.connect(
            sigc::mem_fun(*this, &RecursiveDescentDisassembler::slotStateKill));

    if (m_verbose)  {
        s2e()->getDebugStream() << "[RecursiveDescentDisassembler] Plugin initialized" << '\n';
    }
}

std::vector< std::pair< uint64_t, uint64_t> > RecursiveDescentDisassembler::getConstantMemoryRanges()
{
	std::vector< std::pair< uint64_t, uint64_t> > ranges;
	bool ok;
	std::vector<std::string> keys = s2e()->getConfig()->getListKeys(getConfigKey() + ".constantMemoryRanges", &ok);
	if (ok) {
		for (std::vector<std::string>::const_iterator itr = keys.begin(); itr != keys.end(); itr++)  {
			bool addr_ok;
			bool size_ok;
			uint64_t address = s2e()->getConfig()->getInt(getConfigKey() + ".constantMemoryRanges." + *itr + ".address", 0, &addr_ok);
			uint64_t size = s2e()->getConfig()->getInt(getConfigKey() + ".constantMemoryRanges." + *itr + ".size", 0, &size_ok);

			if (!(addr_ok && size_ok))  {
					continue;
			}

			ranges.push_back(std::make_pair(address, size));
		}
	}

	return ranges;
}

bool RecursiveDescentDisassembler::isValidCodeAccess(S2EExecutionState* state, uint64_t addr)
{
	return is_valid_code_addr(state->getConcreteCpuState(), addr);
}

void RecursiveDescentDisassembler::initializeWithState(S2EExecutionState* state)
{
#ifdef TARGET_WORDS_BIGENDIAN
	//bool bigEndian = true;
#else
    bool bigEndian = false;
#endif

    m_functionPassManager = new llvm::FunctionPassManager(reinterpret_cast<llvm::Function *>(state->getTb()->llvm_function)->getParent());
#ifndef TARGET_WORDS_BIGENDIAN
    m_functionPassManager->add(new S2EReplaceConstantLoadsPass(state, getConstantMemoryRanges(), bigEndian));
#endif
    m_functionPassManager->add(llvm::createConstantPropagationPass());
    m_functionPassManager->add(new S2EInlineHelpersPass());
    m_functionPassManager->add(&m_transformPass);
#ifdef TARGET_ARM
    m_functionPassManager->add(&m_armMergePcThumbPass);
#endif
    //TODO: Dead code elimination pass is probably superfluous here
    m_functionPassManager->add(llvm::createDeadCodeEliminationPass());
    m_functionPassManager->doInitialization();
    m_controlFlowGraphRoot = state->getPc();
}

void RecursiveDescentDisassembler::slotTranslateBlockStart(ExecutionSignal *signal,
                                      S2EExecutionState *state,
                                      TranslationBlock *tb,
                                      uint64_t pc)
{
    if (m_verbose)  {
        s2e()->getDebugStream() << "[RecursiveDescentDisassembler] Translating block " << hexval(pc) << '\n';
    }
	signal->connect(sigc::mem_fun(*this, &RecursiveDescentDisassembler::slotExecuteBlockStart));
}

void RecursiveDescentDisassembler::slotExecuteBlockStart(S2EExecutionState *state, uint64_t pc)
{
#ifdef TARGET_ARM
	bool thumb = state->readCpuState(CPU_OFFSET(thumb), sizeof(uint32_t) * 8);
#endif

    if (!m_initialized)  {
        initializeWithState(state);
	m_initialized = true;
    }

    if (m_bbsDone.find(pc) == m_bbsDone.end())
    {
        if (m_verbose)  {
        	std::string addenum = "";
#ifdef TARGET_ARM
        	if (thumb)  {
        		addenum = " (thumb)";
        	}
        	else {
        		addenum = " (arm)";
        	}
#endif
            s2e()->getDebugStream() << "[RecursiveDescentDisassembler] Executing block " << hexval(pc) << addenum << '\n';
        }
        m_bbsDone[pc] = true;

		llvm::Function* function = static_cast<llvm::Function*>(state->getTb()->llvm_function);
//		llvm::LLVMContext& ctx = function->getContext();

#if defined(TARGET_ARM)
		if (thumb)  {
//			function->setMetadata("trans.instruction_set", MDNode::get(ctx, MDString::get(ctx, "thumb")));
		}
		else {
//			function->setMetadata("trans.instruction_set", MDNode::get(ctx, MDString::get(ctx, "arm")));
		}
#elif defined(TARGET_I386)
		//TODO: Detect 64 bit mode and set different instruction set?
//		function->setMetadata("trans.instruction_set", MDNode::get(ctx, MDString::get(ctx, "i386")));
#else
#error Unknown instruction set
#endif

        if (m_printFunctionBeforeOptimization)  {
		    s2e()->getDebugStream() << "Original function: " << *function << '\n';
        }


        m_functionPassManager->run(*function);


        if (m_printFunctionAfterOptimization)  {
            s2e()->getDebugStream() << "Transformed function: " << *function << '\n';
        }



        if (m_verbose)
            s2e()->getDebugStream() << "BB " << hexval(pc) <<
                ": PC of last instr + 4: " <<
                hexval(state->getTb()->llvm_first_pc_after_bb) << '\n';

		m_extractAndBuildPass.targetMap.clear();
		m_extractAndBuildPass.setBlockAddr(pc, state->getTb()->llvm_first_pc_after_bb);
		m_extractAndBuildPass.runOnFunction(*function);

		if (m_extractAndBuildPass.targetMap.empty() && !m_extractAndBuildPass.hasImplicitTarget())  {
            s2e()->getDebugStream() << "Block has no successors: " <<
                hexval(pc) << "\n";
		}

		std::vector<uint64_t> targets;

		for (S2EExtractAndBuildPass::targetmap_t::iterator itr = m_extractAndBuildPass.targetMap.begin();
					 itr != m_extractAndBuildPass.targetMap.end();
					 itr++)
		{
			uint64_t target_pc = itr->second;

#ifdef TARGET_ARM
			//If next instruction set is known because the jump instruction explicitly specified it, respect this
			if (m_armMergePcThumbPass.getInstructionSet(target_pc) != S2EArmMergePcThumbPass::UNKNOWN)  {
				if (m_armMergePcThumbPass.getInstructionSet(target_pc) == S2EArmMergePcThumbPass::THUMB) {
					target_pc |= 1;
				}
			}
			//Otherwise use the same instruction set as in the current block
			else if (thumb) {
				target_pc |= 1;
			}
#endif
			targets.push_back(target_pc);
		}

		if (m_extractAndBuildPass.hasImplicitTarget())  {
			uint64_t target_pc = state->getTb()->llvm_first_pc_after_bb;

			//Implicit control flow always uses the same instruction set as the current block
#if defined(TARGET_ARM)
			if (thumb)  {
				target_pc |= 1;
			}
#endif
			targets.push_back(target_pc);
		}




		//Only queue those BBs for analysis that have not been done yet
		//(and are not queued yet, but the list is difficult to search so we don't do it)
		for (std::vector<uint64_t>::iterator itr = targets.begin(); itr != targets.end(); itr++)  {
			if (m_bbsDone.find(*itr) == m_bbsDone.end())  {
				if (isValidCodeAccess(state, *itr))  {
					m_bbsTodo.push_back(*itr);
				}
				else {
					s2e()->getWarningsStream() << "ERROR: translation block " << hexval(pc)
							<< " points outside memory to " << hexval(*itr) << '\n';
				}

			}
		}

		//Store control flow graph information
#if defined(TARGET_ARM)
		std::pair<uint64_t, std::vector<uint64_t> >& bbCfg = m_controlFlowGraph[pc | thumb];
#else
		std::pair<uint64_t, std::vector<uint64_t> >& bbCfg = m_controlFlowGraph[pc];
#endif

		bbCfg.first = state->getTb()->llvm_first_pc_after_bb;
		for (std::vector<uint64_t>::iterator itr = targets.begin(); itr != targets.end(); itr++)  {
			bbCfg.second.push_back(*itr);
		}

		//Print queued nodes to the log
		if (m_verbose) {
			s2e()->getDebugStream() << "List of next targets:";
			for (std::vector<uint64_t>::iterator itr = targets.begin(); itr != targets.end(); itr++)  {
				if (m_bbsDone.find(*itr) == m_bbsDone.end())  {
					s2e()->getDebugStream() << " " << hexval(*itr);
				}
			}
			s2e()->getDebugStream() << '\n';
		}
	}



    if (m_bbsTodo.empty()) {
    	this->exit();
    }
    else {
        uint64_t target;
        while (!m_bbsTodo.empty()) {
            target = m_bbsTodo.front();
            m_bbsTodo.pop_front();

            if (m_bbsDone.find(target) != m_bbsDone.end()) {
                s2e()->getWarningsStream() <<
                    "Already explored: " << toHexString(target) << "\n";
                if (m_bbsTodo.empty()) {
                    this->exit();
                }
            } else {
                break;
            }
        }

    	if (m_verbose)  {
    		s2e()->getDebugStream() << "Starting to handle next basic block: " << hexval(target) << '\n';
    	}

#if defined(TARGET_ARM)
        s2e()->getWarningsStream() <<
            "setting PC to: " << toHexString(target) << "\n";
    	state->setPc(target & -2);
    	state->writeCpuState(CPU_OFFSET(thumb), target & 1, sizeof(uint32_t) * 8);
#else
		state->setPc(target);
#endif
        size_t funcCnt = m_extractAndBuildPass.getFunctionCount();
        if (funcCnt % 5000 == 0 && m_lastCountOfSavedFunctions != funcCnt) {
            // Save every 5000 functions discovered
            s2e()->getDebugStream() <<
                "[RecursiveDescentDisassembler] saving functions: " <<
                    funcCnt << "\n" ;
            m_lastCountOfSavedFunctions = funcCnt;
            saveState(true);
        }

        throw CpuExitException();
    }
}

void RecursiveDescentDisassembler::saveCfgDotFile(const std::string& filename)
{
	std::ofstream fout(filename.c_str());
	fout << "digraph CFG {\n";
	fout << "    \"" << toHexString(m_controlFlowGraphRoot) << "\" [shape=oval,peripheries=2];\n";

	for (std::map< uint64_t, std::pair< uint64_t, std::vector< uint64_t > > >::iterator mapItr = m_controlFlowGraph.begin();
		 mapItr != m_controlFlowGraph.end();
		 mapItr++)
	{
		for (std::vector< uint64_t >::iterator listItr = mapItr->second.second.begin();
			 listItr != mapItr->second.second.end();
			 listItr++)
		{
			fout << "    \"" << toHexString(mapItr->first) << "\" -> \"" << toHexString(*listItr) << "\";\n";
		}
	}

	fout << "}\n";
	fout.close();
}

std::string RecursiveDescentDisassembler::getSaveStateFilename(
        std::string initialFileName,
        bool mightBeInvalid)
{
    std::stringstream ss;

    if (mightBeInvalid)
        ss << std::setw(8) << std::setfill('0') << m_saveID << "-" << initialFileName;
    else
        ss << initialFileName;

    return s2e()->getOutputFilename(ss.str().c_str());
}

void RecursiveDescentDisassembler::saveState(bool mightBeInvalid)
{
    std::string error;

    // Dump the LLVM module
    llvm::raw_fd_ostream llvmOstream(
            getSaveStateFilename("translated_llvm.ll", mightBeInvalid).c_str(),
            error, 0);
    llvmOstream <<  *m_extractAndBuildPass.getBuiltModule();
    llvmOstream.close();

    if (!mightBeInvalid) {
        // Dump generated LLVM module
        llvm::raw_fd_ostream bitcodeOstream(
                getSaveStateFilename("translated_llvm.bc", mightBeInvalid).c_str(),
                error, 0);
        llvm::WriteBitcodeToFile(m_extractAndBuildPass.getBuiltModule(),
                bitcodeOstream);
        bitcodeOstream.close();
    }

    // Advance state counter
    ++m_saveID;
}

void RecursiveDescentDisassembler::exit() {
    s2e()->getDebugStream() << "[RecursiveDescentDisassembler] exiting" << '\n';

	//Dump dot CFG file
	//saveCfgDotFile(s2e()->getOutputFilename("cfg.dot"));

    saveState(false);

	::exit(0);
}

RecursiveDescentDisassembler::~RecursiveDescentDisassembler()
{
    s2e()->getDebugStream() <<
        "[RecursiveDescentDisassembler] destructor\n" ;

    saveState(false);
}

void RecursiveDescentDisassembler::slotStateKill(S2EExecutionState *state)
{
    s2e()->getDebugStream() <<
        "[RecursiveDescentDisassembler] state killed, remaining " <<
        m_bbsTodo.size() << " BBs to explore\n";

    // TODO: we should respawn another state in order to explore
    // the remaining BBs
}

} // namespace plugins
} // namespace s2e
