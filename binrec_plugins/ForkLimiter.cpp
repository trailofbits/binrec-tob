#include <s2e/ConfigFile.h>
#include <s2e/S2E.h>
#include <s2e/Utils.h>
#include <s2e/S2EExecutor.h>

#include "ForkLimiter.h"
#include "ModuleSelector.h"

namespace s2e {
namespace plugins {

//TODO: Limiting forks to only library calls doesn't work properly.
S2E_DEFINE_PLUGIN(ForkLimiter, "Limits how many times each instruction in a module can fork, limits max running time of state",
                     "ForkLimiter", "ModuleSelector");

void ForkLimiter::initialize() {
    m_detector = (ModuleExecutionDetector*)s2e()->getPlugin("ModuleExecutionDetector");

//    m_detector->onModuleTransition.connect(
//        sigc::mem_fun(*this, &ForkLimiter::onModuleTransition));

    s2e()->getCorePlugin()->onTranslateBlockStart.connect(
                sigc::mem_fun(*this,
                        &ForkLimiter::onSlotTranslateBlockStart)
                );

    s2e()->getCorePlugin()->onTimer.connect(sigc::mem_fun(*this, &ForkLimiter::onTimer));

    s2e()->getCorePlugin()->onStateSwitch.connect(
            sigc::mem_fun(*this, &ForkLimiter::onStateSwitch));

    
    ModuleSelector *selector =
        (ModuleSelector*)(s2e()->getPlugin("ModuleSelector"));
//    selector->onModuleExecute.connect(
//            sigc::mem_fun(*this, &ForkLimiter::slotModuleLeave));
//    selector->onModuleExecute.connect(
//            sigc::mem_fun(*this, &ForkLimiter::slotModuleReturn));
    selector->onModuleLoad.connect(
            sigc::mem_fun(*this, &ForkLimiter::slotModuleLoad));
    // Limit of forks per program counter, -1 means don't care
    bool ok;
    m_timeLimit = s2e()->getConfig()->getInt(getConfigKey() + ".maxRunningTime", 10, &ok);
    m_forkLimit = s2e()->getConfig()->getInt(getConfigKey() + ".maxForkCount", 3, &ok);
    if ((int) m_forkLimit != -1) {
        if (m_detector) {
            s2e()->getCorePlugin()->onStateFork.connect(sigc::mem_fun(*this, &ForkLimiter::onFork));
        } else if (ok) {
            s2e()->getWarningsStream() << "maxForkCount requires ModuleExecutionDetector\n";
            exit(-1);
        }
    }

    m_timerTicks = 0;
}

void ForkLimiter::slotModuleLoad(S2EExecutionState *state,
                                 const ModuleDescriptor &module)
{
    s2e()->getDebugStream() << "module.Name: " << module.Name << '\n';
    m_binModule = module;
}


void ForkLimiter::onStateSwitch(S2EExecutionState *currentState,
                                      S2EExecutionState *nextState)
{
    DECLARE_PLUGINSTATE(ForkLimiterState, currentState);
    plgState->timerTicks += m_timerTicks;
    m_timerTicks = 0;

    if(currentState->getID() != 0 && plgState->timerTicks > m_timeLimit){
        plgState->terminate = true;
        plgState->reason = timeout;
        //s2e()->getExecutor()->terminateStateEarly(*currentState, "maxRunningTime is reached");
    } 
    //s2e()->getDebugStream() << "m_runningConcrete: " << currentState->isRunningConcrete() << "\n";

 
    //s2e()->getDebugStream() << "StateID: " << currentState->getID() << " runningTime: " << plgState->timerTicks << "\n"; 
}

void ForkLimiter::slotModuleLeave(S2EExecutionState *state, uint64_t pc)
{
    //DECLARE_PLUGINSTATE(ForkLimiterState, state);
    //s2e()->getDebugStream() << "[ModuleLeave] state:" << state->getID() << "\n";
    //plgState->forkEnabled = false;
}

void ForkLimiter::slotModuleReturn(S2EExecutionState *state, uint64_t pc)
{
    //DECLARE_PLUGINSTATE(ForkLimiterState, state);
    //s2e()->getDebugStream() << "[ModuleReturned] state:" << state->getID() << "\n";
    //plgState->forkEnabled = true;
}

// TODO: switch to onTranslateBlockEnd
// TODO: use ELFSelector signals to not fork in libcalls
// TODO: Start working on omnetpp by tomorrow.
// TODO: 
void ForkLimiter::onSlotTranslateBlockStart(
            ExecutionSignal *signal,
            S2EExecutionState* state,
            TranslationBlock *tb,
            uint64_t pc)
{
    DECLARE_PLUGINSTATE(ForkLimiterState, state);

    if (plgState->terminate) {
        //if(plgState->reason == libFork){
        //    signal->connect(
        //        sigc::mem_fun(*this, &ForkLimiter::onTermination1)
        //    );
        //}
        //else
        if(plgState->reason == forkLimit){
            signal->connect(
                sigc::mem_fun(*this, &ForkLimiter::onTermination2)
            );
        }
        else if(plgState->reason == timeout){
            signal->connect(
                sigc::mem_fun(*this, &ForkLimiter::onTermination3)
            );
        }
    }
}


void ForkLimiter::onTermination1(S2EExecutionState* state, uint64_t sourcePc)
{
    s2e()->getExecutor()->terminateState(*state, "libFork is terminated");
}
void ForkLimiter::onTermination2(S2EExecutionState* state, uint64_t sourcePc)
{
    s2e()->getExecutor()->terminateState(*state, "forkLimit is reached");
}
void ForkLimiter::onTermination3(S2EExecutionState* state, uint64_t sourcePc)
{
    s2e()->getExecutor()->terminateState(*state, "maxRunningTime is reached");
}



void ForkLimiter::onFork(S2EExecutionState *state, const std::vector<S2EExecutionState *> &newStates,
                         const std::vector<klee::ref<klee::Expr> > &newConditions) {


/*
    s2e()->getDebugStream() << "0\n";
    const ModuleDescriptor* module = m_detector->getCurrentDescriptor(state);
    s2e()->getDebugStream() << "1\n";
    if (!module) {
        return;
    }
    s2e()->getDebugStream() << "2\n";

    uint64_t curPc = module->ToNativeBase(state->getPc());
*/

//TODO(aaltinay): getCurrentDescriptor returns null module always. Don't know why.
// For now, it is okay since we have only one module.

    //kill forked states in lib function
    //DECLARE_PLUGINSTATE(ForkLimiterState, state);
    //s2e()->getWarningsStream() << "[onFork] state:" << state->getID() << 
    //                                  " forkEnabled: " << state->isForkingEnabled() << "\n"; 
    if(!m_binModule.Contains(state->regs()->getPc())){
        for(size_t i = 0, e = newStates.size(); i < e; ++i){
            //DECLARE_PLUGINSTATE(ForkLimiterState, newStates[i]);
           
            //s2e()->getWarningsStream() << "[[onFork]] newState:" << newStates[i]->getID() << 
                                      //" forkEnabled: " << plgState->forkEnabled << "\n"; 
            //                          " forkEnabled: " << newStates[i]->isForkingEnabled() << "\n"; 
            if(state->getID() == newStates[i]->getID()){
               //s2e()->getWarningsStream() << "[[onFork]] newState:" << newStates[i]->getID() << 
               //                       " forkEnabled: " << newStates[i]->isForkingEnabled() << "\n"; 
                continue;
            }
            newStates[i]->disableForking();
            //s2e()->getWarningsStream() << "[[onFork]] newState:" << newStates[i]->getID() << 
            //                          " forkEnabled: " << newStates[i]->isForkingEnabled() << "\n"; 
//            plgState->terminate = true;
//            plgState->reason = libFork;
        }
        return;
    }
/*else{
        for(size_t i = 0, e = newStates.size(); i < e; ++i){
            DECLARE_PLUGINSTATE(ForkLimiterState, newStates[i]);
            plgState->forkEnabled = true;
            s2e()->getWarningsStream() << "[onFork] newState:" << newStates[i]->getID() << 
                                      " forkEnabled: " << newStates[i]->isForkingEnabled() << "\n"; 
//            plgState->terminate = true;
//            plgState->reason = libFork;
        }


    }
*/
/*    
    if(plgState->forkEnabled == false){
        for(size_t i = 0, e = newStates.size(); i < e; ++i){
            DECLARE_PLUGINSTATE(ForkLimiterState, newStates[i]);
            plgState->terminate = true;
            plgState->forkEnabled = false;
            plgState->reason = libFork;
        }
        return;
    }
*/
    uint64_t curPc = state->regs()->getPc();
    //s2e()->getDebugStream() << "3\n";
    ++m_forkCount["module"][curPc];
    //s2e()->getDebugStream() << "4\n";
    if (m_forkCount["module"][curPc] > m_forkLimit) {
        //s2e()->getDebugStream() << "StateID: " << state->getID() << " forkLimit reached\n";
        for(size_t i = 0, e = newStates.size(); i < e; ++i){
            DECLARE_PLUGINSTATE(ForkLimiterState, newStates[i]);
            plgState->terminate = true;
            plgState->reason = forkLimit;
        }
    }
}

void ForkLimiter::onTimer() {
    ++m_timerTicks;
}

ForkLimiterState::ForkLimiterState() : timerTicks(0), terminate(false),  reason(noReason) {}

ForkLimiterState::~ForkLimiterState() {}


} // namespace plugins
} // namespace s2e

