#include "TestCaseLog.h"
#include <cassert>
#include <ctype.h>
#include <s2e/ConfigFile.h>
#include <s2e/CorePlugin.h>
#include <s2e/S2E.h>
#include <s2e/S2EExecutionState.h>
#include <s2e/S2EExecutor.h>
#include <s2e/Utils.h>

namespace s2e {
    namespace plugins {

        typedef std::vector<unsigned char> value_t;
        typedef std::pair<std::string, value_t> pair_t;

        S2E_DEFINE_PLUGIN(TestCaseLog, "Log concrete inputs for testcases", "TestCaseLog");

        void TestCaseLog::initialize()
        {
            std::string logFileName =
                s2e()->getConfig()->getString(getConfigKey() + ".logFile", "testcases.txt");
            m_logFile.open(s2e()->getOutputFilename(logFileName).c_str());
            assert(m_logFile);

            m_onlyPrintable = s2e()->getConfig()->getBool(getConfigKey() + ".onlyPrintable", true);

            s2e()->getCorePlugin()->onTestCaseGeneration.connect(
                sigc::mem_fun(*this, &TestCaseLog::slotTestCaseGeneration));
        }

        TestCaseLog::~TestCaseLog()
        {
            m_logFile.close();
        }

        static std::string stripName(const std::string &name)
        {
            size_t left = name.find_first_of('_') + 1;
            size_t right = name.find_last_of('_');
            return name.substr(left, right - left);
        }

        static bool getPrintableString(const value_t &value, std::string &s)
        {
            s = "";

            for (int i = 0; i < value.size(); i++) {
                unsigned char byte = value[i];

                if (!byte)
                    break;
                else if (byte == '\n')
                    s.append("\\n");
                else if (byte == '\b')
                    s.append("\\b");
                else if (byte == '\r')
                    s.append("\\r");
                else if (byte == '\t')
                    s.append("\\t");
                else if (!isprint(byte))
                    return false;
                else
                    s.push_back(byte);
            }

            return true;
        }

        void
        TestCaseLog::slotTestCaseGeneration(S2EExecutionState *state, const std::string &message)
        {
            std::vector<pair_t> out;
            bool success = s2e()->getExecutor()->getSymbolicSolution(*state, out);

            if (!success) {
                s2e()->getWarningsStream()
                    << "could not get symbolic solutions for state " << state->getID() << '\n';
                return;
            }

            std::string buffer;
            bool bufferSet = false;
            std::vector<std::string> args;

            foreach2(input, out.begin(), out.end())
            {
                const std::string name = stripName(input->first);
                std::string value;

                if (!name.compare(0, 3, "arg")) {
                    int index = atoi(name.substr(3).c_str());

                    if (args.size() <= index)
                        args.resize(index + 1);

                    if (!getPrintableString(input->second, args[index])) {
                        s2e()->getDebugStream(state)
                            << "unprintable characters in " << input->first << '\n';
                        return;
                    }
                } else if (name == "buffer") {
                    bufferSet = true;
                    if (!getPrintableString(input->second, buffer)) {
                        s2e()->getDebugStream(state)
                            << "unprintable characters in " << input->first << '\n';
                        return;
                    }
                } else if (name != "n_args") {
                    s2e()->getWarningsStream(state)
                        << "unexpected name: " << input->first << " (" << name << ")\n";
                    continue;
                }
            }

            // m_logFile << state->getID() << ": ";
            // if (bufferSet)
            //    m_logFile << "(stdin: \"" << buffer << "\") ";
            // if (args.size() == 0) {
            //    m_logFile << "(no args)";
            //} else {
            //    m_logFile << "(args:";
            //    foreach2(it, args.begin(), args.end())
            //        m_logFile << " \"" << *it << '"';
            //    m_logFile << ')';
            //}
            // m_logFile << '\n';

            // m_logFile << "  ";
            if (bufferSet)
                m_logFile << "printf \"" << buffer << "\" | ";
            m_logFile << "./binary";
            foreach2(it, args.begin(), args.end()) m_logFile << " \"" << *it << '"';
            m_logFile << '\n';

            m_logFile.flush();
        }

    } // namespace plugins
} // namespace s2e
