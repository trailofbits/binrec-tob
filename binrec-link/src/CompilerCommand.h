#ifndef BINREC_COMPILERCOMMAND_H
#define BINREC_COMPILERCOMMAND_H

#include <llvm/ADT/StringRef.h>
#include <system_error>
#include <vector>

namespace binrec {
enum class CompilerCommandMode { Compile, Link };

class CompilerCommand {
    CompilerCommandMode m_mode;

    llvm::StringRef m_compilerExe = "/usr/bin/g++";
    llvm::StringRef m_outputPath;
    std::vector<llvm::StringRef> m_inputPaths;

    llvm::StringRef m_linkerScriptPath;

public:
    explicit CompilerCommand(CompilerCommandMode mode) : m_mode(mode) {}

    auto outputPath() -> llvm::StringRef & { return m_outputPath; }
    auto inputPaths() -> std::vector<llvm::StringRef> & { return m_inputPaths; }

    auto linkerScriptPath() -> llvm::StringRef & { return m_linkerScriptPath; }

    auto run() -> std::error_code;
};
} // namespace binrec

#endif
