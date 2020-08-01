#include <cassert>

#include <s2e/S2E.h>
#include <s2e/Utils.h>
#include <s2e/ConfigFile.h>
#include <s2e/Plugins/CorePlugin.h>

#include "FunctionLog.h"
#include "ModuleSelector.h"


#include <s2e/Plugins/ModuleExecutionDetector.h>

#include <llvm/LLVMContext.h>
#include <llvm/Function.h>
#include <llvm/Metadata.h>
#include <llvm/Constants.h>
#include "util.h"
//#include <string>

namespace s2e {
namespace plugins {

S2E_DEFINE_PLUGIN(FunctionLog,
                  "Log register values at basic block start (don't use with symbolic execution)",
                  "FunctionLog", "ModuleSelector");

#ifdef TARGET_I386

void FunctionLog::initialize()
{
    std::string logFileName = s2e()->getConfig()->getString(
            getConfigKey() + ".logFile", "function-log.txt");
    m_logFile.open(s2e()->getOutputFilename(logFileName).c_str());
    assert(m_logFile);

/*    ModuleSelector *selector =
        (ModuleSelector*)(s2e()->getPlugin("ModuleSelector"));
    m_connExec = selector->onModuleExecute.connect(
            sigc::mem_fun(*this, &RegisterLog::writeBBHeader));
    m_connExec = selector->onTransInstEnd.connect(
            sigc::mem_fun(*this, &RegisterLog::writeLogLine));
    m_connFork = s2e()->getCorePlugin()->onStateFork.connect(
            sigc::mem_fun(*this, &RegisterLog::slotStateFork));
    m_connFork = s2e()->getCorePlugin()->onTranslateRegisterAccessEnd.connect(
            sigc::mem_fun(*this, &RegisterLog::onTranslateRegisterAccessEnd));
*/
    //m_baseDirs = s2e()->getConfig()->getStringList(getConfigKey() + ".baseDirs");

    //m_exportInterval =
    //    s2e()->getConfig()->getInt(getConfigKey() + ".exportInterval", 0);

    ModuleSelector *selector =
        (ModuleSelector*)(s2e()->getPlugin("ModuleSelector"));
    selector->onModuleLoad.connect(
            sigc::mem_fun(*this, &FunctionLog::slotModuleLoad));
    selector->onModuleExecute.connect(
            sigc::mem_fun(*this, &FunctionLog::slotModuleExecute));
    //selector->onTransInstEnd.connect(
    //        sigc::mem_fun(*this, &ExportELF::slotOnTranslateInstructionEnd));
    selector->onModuleLeave.connect(
            sigc::mem_fun(*this, &FunctionLog::slotModuleLeave));
    selector->onModuleReturn.connect(
            sigc::mem_fun(*this, &FunctionLog::slotModuleReturn));

    ModuleExecutionDetector *modEx =
        (ModuleExecutionDetector*)(s2e()->getPlugin("ModuleExecutionDetector"));
    modEx->onModuleSignal.connect(
            sigc::mem_fun(*this, &FunctionLog::slotModuleSignal));

    //s2e()->getCorePlugin()->onStateFork.connect(
    //        sigc::mem_fun(*this, &ExportELF::slotStateFork));

    m_functionMonitor = static_cast<FunctionMonitor*>(s2e()->getPlugin("FunctionMonitor"));

    s2e()->getDebugStream() << "[FunctionLog] Plugin initialized\n";
    callerPc = 0;
}

FunctionLog::~FunctionLog()
{

    s2e()->getMessagesStream() << "saving Function Metadata... ";

    std::string error;
    for (std::map<uint32_t, std::set<uint32_t> >::iterator it=entryPcToBBPcs.begin(), et=entryPcToBBPcs.end(); it!=et; ++it){
        std::ostringstream ss;
        ss << hexval(it->first);
        //std::string funcPc = hexval(it->first);
        std::string funcFile("func_" + ss.str());
        raw_fd_ostream funcOstream(s2e()->getOutputFilename(funcFile).c_str(), error, 0);
        m_logFile << funcFile << '\n';
        for (std::set<uint32_t>::iterator i=(it->second).begin(), e=(it->second).end(); i!=e; ++i){
            const uint32_t bbPc = *i;
            funcOstream.write((const char*)&bbPc, sizeof (uint32_t));
            m_logFile << bbPc << '\n';
        }
        funcOstream.close();
    }
    
    raw_fd_ostream entryToCallerOstream(s2e()->getOutputFilename("entryToCaller").c_str(), error, 0);
    m_logFile << "entryToCaller" << '\n';
    for (std::set<uint64_t>::iterator it=entryPcToCallerPc.begin(), et=entryPcToCallerPc.end(); it!=et; ++it){
        const uint64_t entry = *it;
        entryToCallerOstream.write((const char*)&entry, sizeof (uint64_t));
        m_logFile << entry << '\n';
    }
    entryToCallerOstream.close();
    
    raw_fd_ostream entryToReturnOstream(s2e()->getOutputFilename("entryToReturn").c_str(), error, 0);
    m_logFile << "entryToReturn" << '\n';
    for (std::set<uint64_t>::iterator it=entryPcToReturnPc.begin(), et=entryPcToReturnPc.end(); it!=et; ++it){
        const uint64_t entry = *it;
        entryToReturnOstream.write((const char*)&entry, sizeof (uint64_t));
        m_logFile << entry << '\n';
    }
    entryToReturnOstream.close();

    raw_fd_ostream callerToFollowUpOstream(s2e()->getOutputFilename("callerToFollowUp").c_str(), error, 0);
    m_logFile << "callerToFollowUp" << '\n';
    for (std::set<uint64_t>::iterator it=callerPcToRetFollowUp.begin(), et=callerPcToRetFollowUp.end(); it!=et; ++it){
        const uint64_t entry = *it;
        callerToFollowUpOstream.write((const char*)&entry, sizeof (uint64_t));
        m_logFile << entry << '\n';
    }
    callerToFollowUpOstream.close();
    
    s2e()->getMessagesStream() << "done\n";

    m_logFile << "callStack size: " << callStack.size() << '\n';
    while(!callStack.empty()){
        uint32_t a = callStack.top();
        callStack.pop();
        m_logFile << "stack element: " << a << '\n';
    }
        
    //m_logFile << "Destructiing" << functoCaller.size() << std::endl;
    //for (std::multimap<uint64_t, uint32_t>::iterator it=funcToCaller.begin(); it!=funcToCaller.end(); ++it){
    //    m_logFile << (*it).first << " => " << (*it).second << '\n';
    //}
/*
    for(auto& pair : funcToCaller){
        m_logFile << pair.first << " " << pair.second << std::endl;
    }
*/    m_logFile.close();
}

void FunctionLog::slotModuleLoad(S2EExecutionState *state,
                               const ModuleDescriptor &module)
{
    s2e()->getMessagesStream() << "==> ModulePid: " << module.Pid << '\n';
    m_logFile << "==> ModulePid: " << module.Pid << '\n';
     
    //DECLARE_PLUGINSTATE(ExportELFState, state);
    //This is just to make compiler happy.
    //plgState->moduleAttached = false;
    //if(!plgState->moduleAttached){
        FunctionMonitor::CallSignal *cs = m_functionMonitor->getCallSignal(state, -1/*all-the-calls*/, module.Pid);
        cs->connect(sigc::mem_fun(*this, &FunctionLog::onFunctionCall));
        logEnabled = false;
        doExport = true;
        callStack.push(123);
    //} 
}

void FunctionLog::onFunctionCall(S2EExecutionState* state, FunctionMonitorState *fns)
{
    if(!logEnabled || !doExport)
        return;
    
    uint64_t caller = state->getTb()->pcOfLastInstr;
    uint64_t pc = state->getPc();
//    s2e()->getMessagesStream() << "==> onFunctionCall: pc=" << pc << " caller=" << caller << '\n';
//    m_logFile << "==> onFunctionCall: pc=" << pc << " caller=" << caller << '\n';
    //<< " callerNativeToBase=" << hexval(mod->ToNativeBase(caller)) << '\n';
   
    callStack.push(pc); 
    FUNCMON_REGISTER_RETURN_A(state, fns, FunctionLog::onFunctionReturn, caller, pc);
}

void FunctionLog::onFunctionReturn(S2EExecutionState *state, uint64_t func_caller, uint64_t func_begin)
{
    //if(!logEnabled || !doExport)
    if(!doExport)
        return;
   
    //uint64_t caller = state->getTb()->pcOfLastInstr;
    //uint64_t pc = state->getPc();
//    s2e()->getMessagesStream() << "<== onFunctionReturn: return_caller=" << executedBBPc << " func_caller=" << func_caller << " func_begin=" << func_begin << '\n';
//    m_logFile << "<== onFunctionReturn: return_caller=" << executedBBPc << " func_caller=" << func_caller << " func_begin=" << func_begin << '\n';
    //" callerNativeToBase=" << hexval(mod->ToNativeBase(caller)) << '\n';
     
    uint32_t top = callStack.top();
    if(top != func_begin){
        m_logFile << "[Couldn't match caller func: " << top << " with returned func: " << func_begin << "]\n";
        return;
    }
    

    callerPc = func_caller << 32;
//    m_logFile << "Find match: " << top << '\n';
    uint64_t entryToCaller = (func_begin << 32) | (func_caller & 0xffffffff);
    uint64_t entryToReturn = (func_begin << 32) | (executedBBPc & 0xffffffff);
    entryPcToCallerPc.insert(entryToCaller);
    entryPcToReturnPc.insert(entryToReturn);
    callStack.pop();
    
/*    uint32_t top = callStack.top();
    while(top != func_begin){
        callStack.pop();
        uint32_t prev = callStack.top();
        entryPcToBBPcs[prev].insert(entryPcToBBPcs[top].begin(), entryPcToBBPcs[top].end());
        top = prev;
    }
    callStack.pop();
*/
    //funcToCaller.insert(std::pair<uint64_t, uint32_t>(func, func_caller));
}

void FunctionLog::slotModuleLeave(S2EExecutionState *state, uint64_t pc)
{
//    s2e()->getMessagesStream() << "{ModuleLeave}\n";
//    m_logFile << "{ModuleLeave}\n";
    logEnabled = false;
}

void FunctionLog::slotModuleReturn(S2EExecutionState *state, uint64_t pc)
{
//    s2e()->getMessagesStream() << "{ModuleReturn}\n";
//    m_logFile << "{ModuleReturn}\n";
    logEnabled = true;
}

void FunctionLog::slotModuleExecute(S2EExecutionState *state, uint64_t pc)
{
    if(!logEnabled || !doExport)
        return;
    
    executedBBPc = pc;
//    s2e()->getMessagesStream() << "{ModuleExecute}\n";
//    m_logFile << "{ModuleExecute}\n";

//    s2e()->getMessagesStream(state) << "BB_start: " << hexval(pc) << "\n";
//    m_logFile << "BB_start: " << hexval(pc) << "\n";
    
    if(!callStack.empty()){
        entryPcToBBPcs[callStack.top()].insert(pc);
        if(callerPc){
            callerPcToRetFollowUp.insert(callerPc | (pc & 0xffffffff));
            callerPc = 0;
        }
    }else{
        m_logFile << "callStack is empty: " << hexval(pc) << "\n";
    }
}

void FunctionLog::slotModuleSignal(S2EExecutionState *state, const ModuleExecutionCfg &desc)
{
    if (desc.moduleName == "init_env.so") {
        doExport = false;
    }
}
#endif

/*
#ifdef TARGET_I386
static uint32_t cpuVal(S2EExecutionState *state, unsigned offset)
{
    uint32_t value;
    state->readRegisterConcrete(state->getConcreteCpuState(),
            offset, (uint8_t*)&value, sizeof(value));
    return value;
}
#endif

void RegisterLog::onTranslateRegisterAccessEnd(
             ExecutionSignal *signal, S2EExecutionState* state, TranslationBlock* tb,
             uint64_t pc, uint64_t rmask, uint64_t wmask, bool accessesMemory)
{
#ifdef TARGET_I386
    if ((wmask & (1 << R_ESP))) {
        if (tb->s2e_tb_type == TB_CALL || tb->s2e_tb_type == TB_CALL_IND) {
            m_logFile << "call at pc: " <<  hexval(pc) << '\n';
        }
        m_logFile << "R_ESP is modified at pc: " <<  hexval(pc) << '\n';
    }
    m_logFile << "Some register is modified at pc: " <<  hexval(pc) << '\n';
#endif
}

void RegisterLog::writeBBHeader(S2EExecutionState *state, uint64_t pc){
    m_logFile << "BB: " << hexval(pc) << '\n';
}


void RegisterLog::writeLogLine(S2EExecutionState *state, uint64_t pc)
{
    //assert(state->getPc() == pc);
    m_logFile << "state->getPc(): " << hexval(state->getPc()) << "  pc: " << hexval(pc) << '\n';
    m_logFile << "state->getSp(): " << hexval(state->getSp()) << '\n';

#define PRT_VAL(prefix, value) \
    m_logFile << prefix "=" << std::setw(8) << hexval(value);

#ifdef TARGET_I386
    m_logFile << std::left;
    PRT_VAL("pc",      pc);
    PRT_VAL(" eax",    cpuVal(state, CPU_REG_OFFSET(R_EAX)));
    PRT_VAL(" ebx",    cpuVal(state, CPU_REG_OFFSET(R_EBX)));
    PRT_VAL(" ecx",    cpuVal(state, CPU_REG_OFFSET(R_ECX)));
    PRT_VAL(" edx",    cpuVal(state, CPU_REG_OFFSET(R_EDX)));
    PRT_VAL(" ebp",    cpuVal(state, CPU_REG_OFFSET(R_EBP)));
    PRT_VAL(" esp",    cpuVal(state, CPU_REG_OFFSET(R_ESP)));
    PRT_VAL(" esi",    cpuVal(state, CPU_REG_OFFSET(R_ESI)));
    PRT_VAL(" edi",    cpuVal(state, CPU_REG_OFFSET(R_EDI)));
    PRT_VAL(" cc_src", cpuVal(state, CPU_OFFSET(cc_src)));
    PRT_VAL(" cc_dst", cpuVal(state, CPU_OFFSET(cc_dst)));
    PRT_VAL(" cc_op",  cpuVal(state, CPU_OFFSET(cc_op)));
    PRT_VAL(" df",     state->readCpuState(CPU_OFFSET(df), 32));
    PRT_VAL(" mflags", state->readCpuState(CPU_OFFSET(mflags), 32));
    m_logFile << '\n';
#endif

#undef PRT_VAL
}

void RegisterLog::slotStateFork(S2EExecutionState *state,
        const std::vector<S2EExecutionState*> &newStates,
        const std::vector<klee::ref<klee::Expr> > &newCondition)
{
    // don't log if symbolic execution is enabled to prevent concretizing
    s2e()->getDebugStream(state) << "[RegisterLog] stopping register "
        "logging because symbolic execution is enabled\n";
    m_logFile << "stopped logging because of state fork\n";
    m_connExec.disconnect();
    m_connFork.disconnect();
}
*/

} // namespace plugins
} // namespace s2e
