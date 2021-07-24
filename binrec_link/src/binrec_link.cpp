#include "elf_exe_to_obj.hpp"
#include "section_link.hpp"
#include "stitch.hpp"
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/raw_ostream.h>

using namespace binrec;
using namespace llvm;
using namespace llvm::cl;
using namespace llvm::sys::fs;
using namespace std;

static opt<string> Original_Filename{"b", Required, desc("<input original binary>")};
static opt<string> Recovered_Filename("r", Required, desc("<input recovered object>"));
static opt<string> Librt_Filename("l", Required, desc("<binrec runtime library>"));
static opt<string> Output_Filename("o", Required, desc("<output binary>"));
static opt<string> Ld_Script_Filename("t", Required, desc("<linker script>"));

static auto run(LinkContext &ctx) -> Error
{
    SmallString<128> original_object_filename = ctx.work_dir;
    original_object_filename += "/original.o";

    auto sections_or_err = elf_exe_to_obj(original_object_filename, ctx);
    if (error_code ec = sections_or_err.getError()) {
        return errorCodeToError(ec);
    }
    auto &sections = sections_or_err.get();

    SmallString<128> temp_output_path = ctx.work_dir;
    temp_output_path += "/output";

    // Link libgcc statically for division builtins
    if (auto err = link_recovered_binary(
            sections,
            Ld_Script_Filename,
            temp_output_path,
            {Recovered_Filename,
             Librt_Filename,
             original_object_filename,
             "-static-libgcc",
             "-lgcc"},
            ctx))
    {
        return err;
    }

    if (error_code ec = stitch(temp_output_path, ctx)) {
        return errorCodeToError(ec);
    }

    if (error_code ec = copy_file(temp_output_path, Output_Filename)) {
        return errorCodeToError(ec);
    }

    setPermissions(Output_Filename, *getPermissions(temp_output_path));

    return Error::success();
}

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

    int status_code = 0;
    if (auto err = run(ctx)) {
        errs() << err << '\n';
        status_code = 1;
    }

    error_code ec;
    for (directory_iterator temp_dir{ctx.work_dir, ec}, temp_dir_end; temp_dir != temp_dir_end;
         temp_dir.increment(ec))
    {
        if (ec) {
            errs() << ec.message() << '\n';
            break;
        }
        ec = remove(temp_dir->path());
        if (ec) {
            errs() << ec.message() << '\n';
        }
    }

    ec = remove(ctx.work_dir);
    if (ec) {
        errs() << ec.message() << '\n';
    }

    return status_code;
}
