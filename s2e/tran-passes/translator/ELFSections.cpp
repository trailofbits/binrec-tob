#include <math.h>
#include <bfd.h>
#include <limits.h>
#include <stdlib.h>

#include "ELFSections.h"
#include "SectionUtils.h"
#include "PassUtils.h"

using namespace llvm;

char ELFSections::ID = 0;
static RegisterPass<ELFSections> X("elf-sections",
        "S2E Add metadata and global arrays for the old ELF file sections, "
        "and print their loadbase for linking", false, false);

extern "C" {
static void check_bfd(bool condition, const char *message) {
    if (!condition) {
        bfd_perror(message);
        exit(1);
    }
}
}

bool ELFSections::runOnModule(Module &m) {
    bfd_init();

    StringRef sourcePath = getSourcePath(m);
    char path[PATH_MAX];
    if (!realpath(sourcePath.data(), path)) {
        perror(sourcePath.data());
        exit(1);
    }

    DBG("opening binary " << path);
    bfd *obj = bfd_openr(path, NULL);
    check_bfd(obj, "bfd_openr");

    check_bfd(bfd_check_format(obj, bfd_object), "format check");
    DBG("target: " << obj->xvec->name);

    assert(obj->xvec->flavour == bfd_target_elf_flavour);

    for (asection *s = obj->sections; s; s = s->next) {
        // dunno what to do with different LMA so just fail for now
        assert(bfd_section_lma(obj, s) == bfd_section_vma(obj, s));

        section_meta_t meta = {
            .md = NULL,
            .name = bfd_section_name(obj, s),
            .loadBase = (size_t)bfd_section_vma(obj, s),
            .size = (size_t)bfd_section_size(obj, s),
            .fileOffset = (size_t)s->filepos,
            .global = NULL
        };

        if (COPY_SECTIONS.find(meta.name) != COPY_SECTIONS.end()) {
            flagword flags = bfd_get_section_flags(obj, s);
            uint8_t *data = NULL;

            if (flags & SEC_HAS_CONTENTS)
                check_bfd(bfd_malloc_and_get_section(obj, s, &data), "get section contents");

            copySection(m, meta, data, flags & SEC_READONLY);
            writeSectionMeta(m, meta);
        }
    }

    return true;
}
