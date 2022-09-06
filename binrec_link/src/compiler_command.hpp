#ifndef BINREC_COMPILER_COMMAND_HPP
#define BINREC_COMPILER_COMMAND_HPP

#include <llvm/ADT/StringRef.h>
#include <string>
#include <system_error>
#include <vector>

namespace binrec {
    enum class CompilerCommandMode { Compile, Link };

    class CompilerCommand {
    public:
        explicit CompilerCommand(CompilerCommandMode mode);
        auto run() -> std::error_code;

        std::string output_path;
        std::vector<std::string> input_paths;
        std::string linker_script_path;
        bool harden;

    private:
        CompilerCommandMode mode;
        std::string compiler_exe = "/usr/bin/g++";
    };
} // namespace binrec

#endif
