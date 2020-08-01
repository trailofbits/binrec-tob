/* vim: set expandtab ts=4 sw=4: */
/*
 * S2E Selective Symbolic Execution Framework
 *
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
 *    Vitaly Chipounov <vitaly.chipounov@epfl.ch>
 *    Volodymyr Kuznetsov <vova.kuznetsov@epfl.ch>
 *
 * All contributors are listed in the S2E-AUTHORS file.
 */

#ifndef S2E_PLUGINS_LLVM_TRANSLATOR_RECURSIVE_DESCENT_DISASSEMBLER_H
#define S2E_PLUGINS_LLVM_TRANSLATOR_RECURSIVE_DESCENT_DISASSEMBLER_H

#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>
#include <s2e/llvm_passes/S2ETransformPass.h>
#include <s2e/llvm_passes/S2EExtractAndBuildPass.h>
#include <s2e/llvm_passes/S2EReplaceConstantLoadsPass.h>
#include <s2e/llvm_passes/S2EARMMergePcThumbPass.h>

#include <map> //TODO: Exchange map with unordered_map to boost efficiency, currently too complicated as we are not using c++11

#include <llvm/PassManager.h>

namespace s2e {
namespace plugins {

struct BasicBlockInfo
{
	BasicBlockInfo(uint64_t pc_start)
		: executed(false),
		  pc_start(pc_start),
		  pc_end(0),
		  static_exit(false) {}
	BasicBlockInfo()
			: executed(false),
			  pc_start(0),
			  pc_end(0),
			  static_exit(false) {}
	bool executed;
	uint64_t pc_start;
	uint64_t pc_end;
	bool static_exit;
};

class RecursiveDescentDisassembler : public Plugin
{
    S2E_PLUGIN
public:
    RecursiveDescentDisassembler(S2E* s2e): Plugin(s2e) {}
    ~RecursiveDescentDisassembler();

    void initialize();
    void initializeWithState(S2EExecutionState* state);
    void slotTranslateBlockStart(ExecutionSignal*, S2EExecutionState *state, 
        TranslationBlock *tb, uint64_t pc);
    void slotExecuteBlockStart(S2EExecutionState* state, uint64_t pc);
    void slotStateKill(S2EExecutionState* state);

private:
    S2ETransformPass m_transformPass;
    S2EExtractAndBuildPass m_extractAndBuildPass;
    S2EArmMergePcThumbPass m_armMergePcThumbPass;
    llvm::FunctionPassManager* m_functionPassManager;
    bool m_initialized;
    //TODO: Exchange map with unordered_map to boost efficiency, currently too complicated as we are not using c++11
    std::map<uint64_t, bool> m_bbsDone;
    std::list<uint64_t> m_bbsTodo;
    bool m_printFunctionBeforeOptimization;
    bool m_printFunctionAfterOptimization;
    bool m_verbose;

    std::map< uint64_t, std::pair< uint64_t, std::vector< uint64_t > > > m_controlFlowGraph;
    uint64_t m_controlFlowGraphRoot;

    void saveCfgDotFile(const std::string& path);
    std::vector< std::pair< uint64_t, uint64_t> > getConstantMemoryRanges();
    bool isValidCodeAccess(S2EExecutionState* state, uint64_t addr);

    int m_saveID;
    size_t m_lastCountOfSavedFunctions;
    void saveState(bool mightBeInvalid = false);
    std::string getSaveStateFilename(std::string name, bool mightBeInvalid = false);

    void exit();

};

} // namespace plugins
} // namespace s2e

#endif // S2E_PLUGINS_LLVM_TRANSLATOR_RECURSIVE_DESCENT_DISASSEMBLER_H
