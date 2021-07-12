#include "CompilerCommand.h"
#include "LinkError.h"
#include <llvm/Support/Program.h>

using namespace binrec;
using namespace llvm;

auto CompilerCommand::run() -> std::error_code {
    std::vector<std::string> argBuffers;
    argBuffers.push_back(m_compilerExe.str());

    std::vector<StringRef> flags = {"-g", "-m32"};
    if (m_mode == CompilerCommandMode::Compile) {
        flags.emplace_back("-c");
    } else if (m_mode == CompilerCommandMode::Link) {
        flags.emplace_back("-no-pie");
    }
    std::transform(flags.begin(), flags.end(), std::back_inserter(argBuffers), [](StringRef s) { return s.str(); });

    if (!linkerScriptPath().empty()) {
        std::string ldScriptArg{"-Wl,-T"};
        ldScriptArg += linkerScriptPath();
        argBuffers.push_back(move(ldScriptArg));
    }

    argBuffers.emplace_back("-o");
    argBuffers.push_back(outputPath().str());

    std::transform(inputPaths().begin(), inputPaths().end(), std::back_inserter(argBuffers),
                   [](StringRef s) { return s.str(); });

    std::vector<StringRef> args{argBuffers.begin(), argBuffers.end()};
    int status = sys::ExecuteAndWait(m_compilerExe, args);
    return status != 0 ? LinkError::CCErr : std::error_code();
}
