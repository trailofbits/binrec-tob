
#include "binrec_link.hpp"
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/raw_ostream.h>

using namespace std;
using namespace llvm;
using namespace llvm::cl;
using namespace llvm::sys::fs;
using namespace binrec;

static opt<string> Original_Filename{"b", Required, desc("<input original binary>")};
static opt<string> Recovered_Filename("r", Required, desc("<input recovered object>"));
static opt<string> Librt_Filename("l", Required, desc("<binrec runtime library>"));
static opt<string> Output_Filename("o", Required, desc("<output binary>"));
static opt<string> Ld_Script_Filename("t", Required, desc("<linker script>"));
static opt<bool> Harden("harden", Required, desc("<harden binary>"));


auto main(int argc, char *argv[]) -> int
{
    llvm_shutdown_obj y;
    ParseCommandLineOptions(argc, argv, "binrec link\n");

    LinkContext ctx;
    if (error_code ec = createUniqueDirectory("binrec_link", ctx.work_dir)) {
        errs() << ec.message() << '\n';
        return 1;
    }

    auto binary = object::createBinary(Original_Filename);
    if (auto err = binary.takeError()) {
        errs() << err << '\n';
        return 1;
    }
    ctx.original_binary = move(binary.get());
    ctx.recovered_filename = Recovered_Filename;
    ctx.librt_filename = Librt_Filename;
    ctx.output_filename = Output_Filename;
    ctx.ld_script_filename = Ld_Script_Filename;
    ctx.harden = Harden;

    int status_code = 0;
    if (auto err = run_link(ctx)) {
        errs() << err << '\n';
        status_code = 1;
    }

    cleanup_link(ctx);

    return status_code;
}
