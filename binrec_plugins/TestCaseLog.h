#ifndef __PLUGIN_TESTCASELOG_H__
#define __PLUGIN_TESTCASELOG_H__

#include <fstream>

#include <s2e/Plugin.h>

namespace s2e {
namespace plugins {

class TestCaseLog : public Plugin
{
    S2E_PLUGIN
public:
    TestCaseLog(S2E* s2e): Plugin(s2e) {}
    ~TestCaseLog();

    void initialize();
    void slotTestCaseGeneration(S2EExecutionState *state,
            const std::string &message);
private:
    std::ofstream m_logFile;
    bool m_onlyPrintable;
};

} // namespace plugins
} // namespace s2e

#endif // __PLUGIN_TESTCASELOG_H__
