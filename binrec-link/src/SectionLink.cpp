#include "SectionLink.h"
#include "CompilerCommand.h"
#include <llvm/Support/Format.h>
#include <llvm/Support/raw_ostream.h>

using namespace binrec;
using namespace llvm;

auto binrec::linkRecoveredBinary(const std::vector<SectionInfo> &sections, StringRef ldScriptPath, StringRef outputPath,
                                 const std::vector<StringRef> inputPaths, LinkContext &ctx) -> llvm::Error {
    auto ldScriptTemplateBufOrErr = MemoryBuffer::getFile(ldScriptPath);
    if (std::error_code ec = ldScriptTemplateBufOrErr.getError()) {
        return errorCodeToError(ec);
    }
    auto &ldScriptTemplateBuf = ldScriptTemplateBufOrErr.get();
    std::string ldScriptTemplate(ldScriptTemplateBuf->getBufferStart(), ldScriptTemplateBuf->getBufferEnd());

    std::string placeholderSegmentsStr = "PLACEHOLDER_SEGMENTS";
    auto placeholderSegmentsBegin = ldScriptTemplate.find(placeholderSegmentsStr);
    assert(placeholderSegmentsBegin != std::string::npos);
    ldScriptTemplate.replace(placeholderSegmentsBegin, placeholderSegmentsStr.size(), "orig PT_LOAD ;");

    std::string sectionsBuf;
    raw_string_ostream sectionsOut{sectionsBuf};
    for (const SectionInfo &section : sections) {
        sectionsOut << "  . = " << format_hex(section.virtualAddress, 8) << " ;\n"
                    << "  .orig" << section.name << " : { *(.orig" << section.name << ") } :orig\n";
    }
    sectionsOut.flush();

    std::string placeholderSectionsStr = "PLACEHOLDER_SECTIONS";
    auto placeholderSectionsBegin = ldScriptTemplate.find(placeholderSectionsStr);
    assert(placeholderSectionsBegin != std::string::npos);
    ldScriptTemplate.replace(placeholderSectionsBegin, placeholderSectionsStr.size(), sectionsBuf);

    SmallString<128> linkerScriptFilename = ctx.workDir;
    linkerScriptFilename.append("/script.ld");
    std::error_code ec;
    raw_fd_ostream linkerScript(linkerScriptFilename, ec);
    if (ec) {
        return errorCodeToError(ec);
    }
    linkerScript << ldScriptTemplate;
    linkerScript.close();

    CompilerCommand cc{CompilerCommandMode::Link};
    cc.linkerScriptPath() = linkerScriptFilename;
    cc.outputPath() = outputPath;
    cc.inputPaths() = inputPaths;

    ec = cc.run();
    if (ec) {
        return errorCodeToError(ec);
    }

    auto binaryOrError = object::createBinary(outputPath);
    if (auto err = binaryOrError.takeError()) {
        return err;
    }
    ctx.recoveredBinary = std::move(binaryOrError.get());

    return Error::success();
}
