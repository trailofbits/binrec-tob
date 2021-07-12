#include "ElfExeToObj.h"
#include "SectionLink.h"
#include "Stitch.h"
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/raw_ostream.h>

using namespace binrec;
using namespace llvm;

namespace {
cl::opt<std::string> originalFilename(cl::Positional, cl::desc("<input original elf>"));
cl::opt<std::string> recoveredFilename(cl::Positional, cl::desc("<input recovered object>"));
cl::opt<std::string> librtFilename(cl::Positional, cl::desc("<binrec runtime library>"));
cl::opt<std::string> outputFilename(cl::Positional, cl::desc("<output merged elf>"));
cl::opt<std::string> ldScriptFilename(cl::Positional, cl::desc("<linker script>"));

auto run(LinkContext &ctx) -> Error {
    SmallString<128> originalObjectFilename = ctx.workDir;
    originalObjectFilename += "/original.o";

    auto sectionsOrErr = elfExeToObj(originalObjectFilename, ctx);
    if (std::error_code ec = sectionsOrErr.getError()) {
        return errorCodeToError(ec);
    }
    auto &sections = sectionsOrErr.get();

    SmallString<128> tempOutputPath = ctx.workDir;
    tempOutputPath += "/output";

    // Link libgcc statically for division builtins
    if (auto err = linkRecoveredBinary(
            sections, ldScriptFilename, tempOutputPath,
            {recoveredFilename, librtFilename, originalObjectFilename, "-static-libgcc", "-lgcc"}, ctx)) {
        return err;
    }

    if (std::error_code ec = stitch(tempOutputPath, ctx)) {
        return errorCodeToError(ec);
    }

    if (std::error_code ec = sys::fs::copy_file(tempOutputPath, outputFilename)) {
        return errorCodeToError(ec);
    }

    sys::fs::setPermissions(outputFilename, *sys::fs::getPermissions(tempOutputPath));

    return Error::success();
}
} // namespace

auto main(int argc, char *argv[]) -> int {
    llvm_shutdown_obj y;
    cl::ParseCommandLineOptions(argc, argv, "binrec link\n");

    LinkContext ctx;
    if (std::error_code ec = sys::fs::createUniqueDirectory("binrec-link", ctx.workDir)) {
        errs() << ec.message() << '\n';
        return 1;
    }
    auto binary = object::createBinary(originalFilename);
    if (auto err = binary.takeError()) {
        errs() << err << '\n';
        return 1;
    }
    ctx.originalBinary = std::move(binary.get());

    int statusCode = 0;
    if (auto err = run(ctx)) {
        errs() << err << '\n';
        statusCode = 1;
    }

    std::error_code ec;
    for (sys::fs::directory_iterator tempDir{ctx.workDir, ec}, tempDirEnd; tempDir != tempDirEnd;
         tempDir.increment(ec)) {
        if (ec) {
            errs() << ec.message() << '\n';
            break;
        }
        ec = sys::fs::remove(tempDir->path());
        if (ec) {
            errs() << ec.message() << '\n';
        }
    }

    ec = sys::fs::remove(ctx.workDir);
    if (ec) {
        errs() << ec.message() << '\n';
    }

    return statusCode;
}
