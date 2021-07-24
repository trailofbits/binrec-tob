#ifndef BINREC_COMPILER_COMMAND_HPP
#define BINREC_COMPILER_COMMAND_HPP

#include <llvm/ADT/StringRef.h>
#include <system_error>
#include <vector>

namespace binrec {
    enum class CompilerCommandMode { Compile, Link };

    class CompilerCommand {
    public:
        explicit CompilerCommand(CompilerCommandMode mode);
        auto run() -> std::error_code;

        llvm::StringRef output_path;
        std::vector<llvm::StringRef> input_paths;
        llvm::StringRef linker_script_path;

    private:
        CompilerCommandMode mode;
        llvm::StringRef compiler_exe = "/usr/bin/g++";
    };
} // namespace binrec

#endif
