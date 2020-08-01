#!/usr/bin/env python
import sys
from ZwoELF import ElfParser, Section, P_type, P_flags


def section_index(f, name):
    for i, s in enumerate(f.sections):
        if s.sectionName == name:
            return i


def find_section_header(f, name, allow_missing=False):
    for s in f.sections:
        if s.sectionName == name:
            return s.elfN_shdr

    if not allow_missing:
        raise ValueError('no such section: ' + name)


def section_contents(f, name):
    h = find_section_header(f, name)
    return f.data[h.sh_offset:h.sh_offset + h.sh_size]


def find_segment_by_type(f, ty):
    for s in f.segments:
        if s.elfN_Phdr.p_type == ty:
            return s

    raise ValueError("no segment with type " + P_type.reverse_lookup[ty])


def alignment_left(addr, alignment):
    return addr % alignment


def alignment_right(addr, alignment):
    mod = addr % alignment
    return alignment - mod if mod else 0


def copy_section(hdr, name):
    s = Section()
    s.sectionName = name
    s.elfN_shdr.sh_name      = hdr.sh_name
    s.elfN_shdr.sh_type      = hdr.sh_type
    s.elfN_shdr.sh_addr      = hdr.sh_addr
    s.elfN_shdr.sh_offset    = hdr.sh_offset
    s.elfN_shdr.sh_size      = hdr.sh_size
    s.elfN_shdr.sh_entsize   = hdr.sh_entsize
    s.elfN_shdr.sh_flags     = hdr.sh_flags
    s.elfN_shdr.sh_link      = hdr.sh_link
    s.elfN_shdr.sh_info      = hdr.sh_info
    s.elfN_shdr.sh_addralign = hdr.sh_addralign
    return s


#TODO I believe this is deprecated in favor of scripts folder routines
if __name__ == '__main__':
    if len(sys.argv) < 4:
        print 'python %s INFILE_OLD INFILE_REC OUTFILE' % sys.argv[0]
        sys.exit(1)

    infile_old = ElfParser(sys.argv[1])
    infile_rec = ElfParser(sys.argv[2])
    outfile = sys.argv[3]

    seg_note = find_segment_by_type(infile_old, P_type.PT_NOTE)
    seg_ehframe = find_segment_by_type(infile_old, P_type.PT_GNU_EH_FRAME)

    sec_text = find_section_header(infile_rec, '.text')
    sec_rodata = find_section_header(infile_rec, '.rodata', True)
    sec_data = find_section_header(infile_rec, '.data', True)
    sec_bss = find_section_header(infile_rec, '.bss')

    copy_rx = section_contents(infile_rec, '.text')
    copy_rw = ''

    if sec_rodata:
        assert sec_rodata.sh_addr > sec_text.sh_addr
        gap_size = sec_rodata.sh_addr - (sec_text.sh_addr + sec_text.sh_size)
        copy_rx += gap_size * '\0' + section_contents(infile_rec, '.rodata')
        print 'zeroed %d-byte gap between .text and .rodata' % gap_size

    if sec_data:
        assert sec_data.sh_addr < sec_bss.sh_addr
        gap_size = sec_bss.sh_addr - (sec_data.sh_addr + sec_data.sh_size)
        copy_rw = section_contents(infile_rec, '.data')
        print 'zeroed %d-byte gap between .data and .bss' % gap_size

    # remove sections that previously mapped to the modified segments
    # add sections with mapping to modified segments for easier debugging
    # NOTE: account for changes in len(infile_old.data) while modifying the
    # section table

    # replace segment-to-section mappings with new sections tpo
    remove_sections = seg_note.sectionsWithin + seg_ehframe.sectionsWithin
    newtext = copy_section(sec_text, ".newtext")
    newbss = copy_section(sec_bss, ".newbss")
    add_sections = [newtext]
    if sec_rodata:
        newrodata = copy_section(sec_rodata, ".newrodata")
        add_sections += [newrodata]
    if sec_data:
        newdata = copy_section(sec_data, ".newdata")
        add_sections += [newdata]
    add_sections += [newbss]
    infile_old.sections = [s for s in infile_old.sections
                           if s not in remove_sections] + add_sections

    # account for shift in section table size
    print 'removing %d sections, adding %d sections' % \
            (len(remove_sections), len(add_sections))
    diff = len(add_sections) - len(remove_sections)
    ehdr = infile_old.header
    # section table must be last bytes of file to avoid shifting existing offsets
    # FIXME
    #assert len(infile_old.data) == ehdr.e_shoff + ehdr.e_shnum * ehdr.e_shentsize
    end = ehdr.e_shoff + ehdr.e_shnum * ehdr.e_shentsize
    newsize = (ehdr.e_shnum + diff) * ehdr.e_shentsize
    diff_bytes = diff * ehdr.e_shentsize
    if diff > 0:
        infile_old.data[end:end] = diff_bytes * '\0'
    else:
        infile_old.data[end + diff_bytes:end] = ''
    ehdr.e_shnum += diff
    ehdr.e_shstrndx = section_index(infile_old, '.shstrtab')

    # change section offsets
    newtext.elfN_shdr.sh_offset = len(infile_old.data)
    if sec_rodata:
        newrodata.elfN_shdr.sh_offset = \
                len(infile_old.data) + len(copy_rx) - sec_rodata.sh_size
    if sec_data:
        newdata.elfN_shdr.sh_offset = len(infile_old.data) + len(copy_rx)
    newbss.elfN_shdr.sh_offset = 0

    # file offsets needs to be page-aligned for the loader not to crash
    alignment = 0x1000

    # make offset page-aligned by adding zeroed padding bytes
    infile_old.data += alignment_right(len(infile_old.data), alignment) * '\0'

    # replace NOTE segment with .text and .rodata sections
    seg_note.elfN_Phdr.p_type = P_type.PT_LOAD
    seg_note.elfN_Phdr.p_offset = len(infile_old.data)
    seg_note.elfN_Phdr.p_vaddr = sec_text.sh_addr
    seg_note.elfN_Phdr.p_paddr = sec_text.sh_addr
    seg_note.elfN_Phdr.p_filesz = len(copy_rx)
    seg_note.elfN_Phdr.p_memsz = len(copy_rx)
    seg_note.elfN_Phdr.p_flags = P_flags.PF_R | P_flags.PF_X
    seg_note.elfN_Phdr.p_align = alignment

    # align again for contents of .data
    gap_size = alignment_right(len(copy_rx), alignment)
    copy_rx += gap_size * '\0'
    print 'adding %d bytes alignment between RX and RW contents' % gap_size

    virtalign = alignment_left(sec_data.sh_addr, alignment)
    copy_rw = virtalign * '\0' + copy_rw
    print 'adding %d bytes virtual alignment before RW vaddr' % virtalign

    # replace GNU_EH_FRAME segment with .data and .bss sections
    seg_ehframe.elfN_Phdr.p_type = P_type.PT_LOAD
    seg_ehframe.elfN_Phdr.p_offset = len(infile_old.data) + len(copy_rx)
    seg_ehframe.elfN_Phdr.p_vaddr = sec_data.sh_addr - virtalign
    seg_ehframe.elfN_Phdr.p_paddr = sec_data.sh_addr - virtalign
    seg_ehframe.elfN_Phdr.p_filesz = len(copy_rw)
    seg_ehframe.elfN_Phdr.p_memsz = \
            sec_bss.sh_addr + sec_bss.sh_size - sec_data.sh_addr
    seg_ehframe.elfN_Phdr.p_flags = P_flags.PF_R | P_flags.PF_W
    seg_ehframe.elfN_Phdr.p_align = alignment

    # TODO: remove sections that previously mapped to the modified segments
    # TODO: add sections with mapping to modified segments for easier debugging
    # NOTE: account for changes in len(infile_old.data) while modifying the
    # section table

    infile_old.data += copy_rx + copy_rw

    infile_old.writeElf(outfile)
