/*
 * Create global array variables all sections except .idata (since a new symbol
 * table is generated)
 */

#include <math.h>

#include "PESections.h"
#include "PEReader.h"
#include "PassUtils.h"
#include "SectionUtils.h"

using namespace llvm;

char PESections::ID = 0;
static RegisterPass<PESections> X("pe-sections",
        "S2E Add global arrays for the old PE file sections, and print their "
        "loadbase for linking", false, false);

static std::string patchSectionName(const std::string &oldName) {
    std::string name(oldName);

    if (name[0] == '.' || name[0] == '/')
        name.replace(0, 1, "__");

    return name;
}

static bool shouldCopySection(const std::string name) {
    return name != ".idata";
}

bool PESections::runOnModule(Module &m) {
    PEReader reader(getSourcePath(m).data());

    // combine raw section table data with runtime load base metadata
    NamedMDNode *mdSections = m.getNamedMetadata("sections");
    assert(mdSections);
    section_meta_t s;
    const Section *lastSection = NULL;
    size_t lastLoadBase;

    fori(i, 0, mdSections->getNumOperands()) {
        readSectionMeta(s, mdSections->getOperand(i));

        const Section *section = reader.findSectionByName(s.name.data());
        assert(section);
#define FLAG(flag) (section->flags & IMAGE_SCN_MEM_##flag)

        s.fileOffset = section->offset;
        s.size = section->virtualSize;

        if (shouldCopySection(s.name)) {
            uint8_t *data = NULL;

            if (s.fileOffset && FLAG(READ)) { // XXX: && !FLAG(DISCARDABLE)) {
                data = reader.readSection(*section);
                assert(data);
            }

            s.name = patchSectionName(s.name);
            copySection(m, s, data, !FLAG(WRITE));
            writeSectionConfig(s.name, s.loadBase);

            if (data)
                delete [] data;
#undef FLAG
        }

        writeSectionMeta(m, s);
        lastSection = section;
        lastLoadBase = s.loadBase;
    }

    // move wrapper function to custom section to avoid overlap
    assert(lastSection);
    assert(lastSection->flags & IMAGE_SCN_ALIGN_4096BYTES);
    unsigned nchunks = ceilf((float)lastSection->rawSize / 0x1000);

    writeSectionConfig(WRAPPER_SECTION, lastLoadBase + nchunks * 0x1000);

    return true;
}
