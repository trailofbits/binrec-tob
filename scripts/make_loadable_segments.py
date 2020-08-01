#!/usr/bin/env python
import sys
from ZwoELF import ElfParser, P_type, P_flags, SH_type
from ZwoELF import D_tag
import subprocess
import re
import os

class BinrecSection:
    def __init__(self, oName):
        self.origName = oName
        self.doPad = True # consider unnecccsary, for deletion
        self.dataStart = False
        self.newName = '.new' + self.origName
    def setRecVaddr(self, va):
        self.recVaddr = int(va, 16)
    def setOrigVaddr(self, va):
        self.origVaddr = int(va, 16)
    def setRecSize(self, s):
        self.recSize = int(s, 16)
    def setRecType(self, s):
        self.recType = s

def find_segment_by_type(f, ty, allow_missing=False):
    for s in f.segments:
        if s.elfN_Phdr.p_type == ty:
            return s

    if not allow_missing:
        raise ValueError("no segment with type " + P_type.reverse_lookup[ty])


def find_section_header(f, name, allow_missing=False):
    for s in f.sections:
        if s.sectionName == name:
            return s
    if not allow_missing:
        raise ValueError('no such section: ' + name)


def copy_rec_fields( sec, secfrom):
    if sec:
        sec.sh_type = secfrom.sh_type
        sec.sh_flags = secfrom.sh_flags
        #link handled separately, info not sure where to source from
        sec.sh_info = secfrom.sh_info
        sec.sh_addralign = secfrom.sh_addralign
        sec.sh_entsize = secfrom.sh_entsize
        # sec.sh_size = secfrom.sh_size

def alignment_v(hdr):
    base = (hdr.sh_addr / 0x1000 ) % 0x10
    return hdr.sh_offset - base * 0x1000 if base else 0


def alignment_right(addr, alignment):
    mod = addr % alignment
    return alignment - mod if mod else 0


def align_section_offset(f, hdr, align):
    # compute shift amount needed for alignment
    old_offset = hdr.sh_offset
    # print hex(hdr.sh_offset)
    #    if align < 0 :
    #        shift = alignment_v(hdr)
    #    else:
    shift = alignment_right(old_offset, align)
    # print hex(shift)
    if not shift:
        return

    # shift subsequent section offsets
    for other in f.sections:
        other = other.elfN_shdr

        if other.sh_offset >= old_offset:
            other.sh_offset += shift

    # shift section header table offset
    if f.header.e_shoff > old_offset:
        f.header.e_shoff += shift

    # insert null padding to shift section contents
    f.data[old_offset:old_offset] = shift * '\0'


#it is not sufficient to set out.dynamicSegmentEntries = rec.dynamicSegmentEntries entries
# DT_NEEDED is not unique, need separate handling,
#assume number of entries is same in out and rec
def modify_dynamic(out,rec):
    out.dynamicSegmentEntries[:] = [x for x in out.dynamicSegmentEntries if x.d_tag != D_tag.DT_NEEDED]
    # assume all needed libraries will be lined by recovered binary, may need to add stubs for fallback only called funcs

    outDynamic = dict()
    for dynEntry in out.dynamicSegmentEntries:
        outDynamic[dynEntry.d_tag]  = dynEntry

    recDynamic = dict()
    for dynEntry in rec.dynamicSegmentEntries:
        recDynamic[dynEntry.d_tag]  = dynEntry
        if dynEntry.d_tag == D_tag.DT_NEEDED:
            out.dynamicSegmentEntries.insert(-1,dynEntry)
            # out.dynamicSegmentEntries.insert(out.dynamicSegmentEntries[-2],dynEntry)

    copyRel = (D_tag.DT_RELSZ, D_tag.DT_REL, D_tag.DT_RELENT)
    copyRela = (D_tag.DT_RELASZ, D_tag.DT_RELA, D_tag.DT_RELAENT)
    knownTagList = (D_tag.DT_PLTRELSZ, D_tag.DT_PLTGOT, D_tag.DT_JMPREL, D_tag.DT_INIT, \
                   D_tag.DT_FINI, D_tag.DT_INIT_ARRAY, D_tag.DT_INIT_ARRAYSZ, \
                   D_tag.DT_FINI_ARRAY, D_tag.DT_FINI_ARRAYSZ, D_tag.DT_GNU_HASH, D_tag.DT_HASH, \
                   D_tag.DT_STRTAB, D_tag.DT_SYMTAB, D_tag.DT_STRSZ, D_tag.DT_SYMENT, \
                   D_tag.DT_VERSYM, D_tag.DT_VERNEED, D_tag.DT_VERNEEDNUM)
                   # D_tag.DT_VERSYM, D_tag.DT_VERNEED, D_tag.DT_VERNEEDNUM, D_tag.DT_NEEDED)

    # print str(recDynamic[D_tag.DT_PLTREL])
    if ( outDynamic[D_tag.DT_PLTREL].d_un == 17 and recDynamic[D_tag.DT_PLTREL].d_un == 17 ):
        for item in copyRel:
            outDynamic[item].d_un = recDynamic[item].d_un
    elif ( outDynamic[D_tag.DT_PLTREL].d_un == 7 and recDynamic[D_tag.DT_PLTREL].d_un == 7 ):
        for item in copyRela:
            outDynamic[item].d_un = recDynamic[item].d_un
    else:
        print "mismatch rel/rela todo" #noqa

    for item in knownTagList:
        if (item in outDynamic and item in recDynamic):
            outDynamic[item].d_un = recDynamic[item].d_un
        elif item in recDynamic:
            # 0x6ffffef5 gnu hash expected
            print "added dynamicEntry from recovered " + str(item)
            out.dynamicSegmentEntries.insert(-1,recDynamic[item])
            # out.dynamicSegmentEntries.append(recDynamic[item])



def fixInfo(secMap, elfHandle, oldSecName, pltOff):
    if oldSecName == '.rel.plt':
        secMap[oldSecName][1].sh_info = pltOff


def fixLink(secMap, elfHandle, oldSecName, dynsymOff, dynstrOff):
    if oldSecName == '.dynsym':
        secMap[oldSecName][1].sh_link = dynstrOff
    elif oldSecName == '.gnu.hash':
        secMap[oldSecName][1].sh_link = dynsymOff
    elif oldSecName == '.gnu.version':
        secMap[oldSecName][1].sh_link = dynsymOff
    elif oldSecName == '.gnu.version_r':
        secMap[oldSecName][1].sh_link = dynstrOff
    elif oldSecName == '.rel.dyn':
        secMap[oldSecName][1].sh_link = dynsymOff
    elif oldSecName == '.rel.plt':
        secMap[oldSecName][1].sh_link = dynsymOff
    elif oldSecName == '.dynamic':
        secMap[oldSecName][1].sh_link = dynstrOff


def processSection(secMap, recBin, outBin, oldSecName, newSecName, dynsymOff, dynstrOff):
    sectionHandle = find_section_header(outBin, newSecName)
    recSecHandle = find_section_header(recBin, oldSecName)
    outOldSecHandle = find_section_header(outBin, oldSecName, True)
    if (outOldSecHandle is not None) and outOldSecHandle.elfN_shdr.sh_type != 0x8: #nobits
        outOldSecHandle.elfN_shdr.sh_type =  0x1  #progbits
    copy_rec_fields(sectionHandle.elfN_shdr, recSecHandle.elfN_shdr)
    secMap[oldSecName] = (newSecName,  sectionHandle.elfN_shdr)
    fixLink(secMap, outBin,  oldSecName, dynsymOff, dynstrOff)
    #it might not be wrong to copy all the sizes from the rec bin to outbin, but it would be redundant with objcopy
    if oldSecName == '.gnu.version':
        sectionHandle.elfN_shdr.sh_size = recSecHandle.elfN_shdr.sh_size
    if recSecHandle.elfN_shdr.sh_type == 0x8 :
        sectionHandle.elfN_shdr.sh_size = recSecHandle.elfN_shdr.sh_size


def doZwoelf(brsList, outfile, recfile,  dataStart, no_link_lift ):
    f = ElfParser(outfile)
    rec = ElfParser(recfile)

    note = find_segment_by_type(f, P_type.PT_NOTE)
    ehframe = find_segment_by_type(f, P_type.PT_GNU_EH_FRAME)
    #[oldname] ->( newname, newsecheader)
    secMap = dict()

    #get index of certain sections
    dynsymOff = None
    dynstrOff = None
    pltOff = None
    if not no_link_lift :
        for i in range(len(f.sections)):
            # print f.sections[i].sectionName
            if f.sections[i].sectionName == '.new.dynsym':
                dynsymOff = i
            elif f.sections[i].sectionName == '.new.plt':
                pltOff = i
            elif f.sections[i].sectionName == '.new.dynstr':
                dynstrOff = i

    for item in brsList:
        processSection(secMap, rec, f, item.origName, item.newName, dynsymOff, dynstrOff)

    if not no_link_lift :
        secMap['.dynamic'] = (None, find_section_header(f,'.dynamic').elfN_shdr)
        fixLink(secMap, f, '.dynamic', dynsymOff, dynstrOff)
        fixInfo(secMap, f, '.rel.plt', pltOff)

    # page-align the first sections in the RX and RW segment so that they can be placed
    # at the start of a segment
    align = 0x1000
    align_section_offset(f, secMap['.text'][1], align)
    align_section_offset(f, secMap[dataStart][1], align)

    # check section order and page alignment
    assert secMap['.text'][1].sh_addr % align == 0

    # replace NOTE segment with .text and .rodata sections
    hdr = note.elfN_Phdr
    hdr.p_type = P_type.PT_LOAD
    hdr.p_offset = secMap['.text'][1].sh_offset
    hdr.p_vaddr = secMap['.text'][1].sh_addr
    hdr.p_paddr = secMap['.text'][1].sh_addr
    # hdr.p_filesz = \
            # secMap['.rodata'][1].sh_size + secMap['.fini'][1].sh_size + secMap['.plt'][1].sh_size + secMap['.init'][1].sh_size \
            # + secMap['.rel.dyn'][1].sh_size + secMap['.rel.plt'][1].sh_size + secMap['.dynsym'][1].sh_size + secMap['.dynstr'][1].sh_size \
            # +secMap['.gnu.version'][1].sh_size + secMap['.gnu.version_r'][1].sh_size \
            # + secMap['.text'][1].sh_size +secMap['.gnu.hash'][1].sh_size
    hdr.p_filesz = \
           secMap['.rodata'][1].sh_offset + secMap['.rodata'][1].sh_size - secMap['.text'][1].sh_offset
    hdr.p_memsz = hdr.p_filesz

    hdr.p_flags = P_flags.PF_R | P_flags.PF_X
    hdr.p_align = align

    # replace GNU_EH_FRAME segment with .data and .bss sections
    hdr = ehframe.elfN_Phdr
    hdr.p_type = P_type.PT_LOAD
    hdr.p_offset =secMap[dataStart][1].sh_offset
    hdr.p_vaddr = secMap[dataStart][1].sh_addr
    hdr.p_paddr = secMap[dataStart][1].sh_addr
    hdr.p_memsz = secMap['.bss'][1].sh_addr + secMap['.bss'][1].sh_size - secMap[dataStart][1].sh_addr
    hdr.p_filesz =secMap['.bss'][1].sh_addr  - secMap[dataStart][1].sh_addr # implicitly 0 size for bss
    hdr.p_flags = P_flags.PF_R | P_flags.PF_W
    hdr.p_align = align

    if not no_link_lift :
        f.header.e_entry = rec.header.e_entry

        # TODO add a relro seg for more security even if wasn't one
        out_relro_seg = find_segment_by_type(f, P_type.PT_GNU_RELRO, True)
        if out_relro_seg is not None:
            out_relro_seg = out_relro_seg.elfN_Phdr
            rec_relro_seg = find_segment_by_type(rec, P_type.PT_GNU_RELRO)
            rec_relro_seg = rec_relro_seg.elfN_Phdr
            #todo copy .dynamic to relrogetment?
            out_relro_seg.p_offset =secMap[dataStart][1].sh_offset
            out_relro_seg.p_vaddr = secMap[dataStart][1].sh_addr
            out_relro_seg.p_paddr = secMap[dataStart][1].sh_addr
            out_relro_seg.p_memsz = secMap['.got.plt'][1].sh_addr - secMap[dataStart][1].sh_addr
            out_relro_seg.p_filesz =out_relro_seg.p_memsz
            out_relro_seg.p_flags = rec_relro_seg.p_flags
            out_relro_seg.p_align = 0x4

        modify_dynamic(f,rec)

        # don't let zwoelf rewrite these sections, we know better
        # we handle this by killing its 'section pointers'
        f.dynamicSymbolEntries = list()
        f.jumpRelocationEntries = list()
        f.relocationEntries = list()
        # f.dynamicSegmentEntries = list()

        # for section in f.sections:
            # print section.elfN_shdr.sh_entsize
    f.writeElf(outfile)


# could make this faster/cooler with fn poiner dispatch, but I don't think it matters
def copySections(brsList, outfile, recfile):
    section_flags = ""
    i = len(brsList) - 1
    while i>=0:
        value = brsList[i]
        filename = "_section_"+value.origName
        outfiledump = ""
        # print filename
        if value.recType == "NOBITS": # handle bss-like empty sections
            args = ("touch _section_"+ value.origName)
            popen = subprocess.Popen(args, shell=True)
        else :
            # if the section is datastart we want to calculate gap before that modification
            gap = brsList[i+1].recVaddr - ( value.recVaddr + value.recSize )
            if value.dataStart:
                align = value.recVaddr % 4096
                value.recVaddr = value.recVaddr - align #this can have part of the rodata section and some rw data section overlapping, but it should only be accessed with r in normal execution, and I don't want to fix it now
                args = ("perl -e \'printf \"\\0\"x\'" + str(align) ) # TODO replace perl with python padding, this is only perl bc legacy
                popen = subprocess.Popen(args, stdout=subprocess.PIPE, shell=True)
                # outfiledump += popen.stdout.read()
                outfiledump += popen.communicate()[0]
                popen.wait()
            args = ("bash $S2EDIR/scripts/dump_section.sh " + recfile + " " + value.origName )
            popen = subprocess.Popen(args, stdout=subprocess.PIPE, shell=True)
            # outfiledump += popen.stdout.read()
            outfiledump += popen.communicate()[0]
            popen.wait()
            # with open(filename, 'wb') as infile:
                # infile.write(popen.stdout.read())
            # ld does unpredictable things if recorded size of sections doesn't match real size, so pad with zeroes
            # if value.doPad: #currently all section should be padded, if last section isnt bss this might fail
            args = ("perl -e \'printf \"\\0\"x\'" +str(gap))
            popen = subprocess.Popen(args,  stdout=subprocess.PIPE, shell=True)
            outfiledump += popen.communicate()[0]
            popen.wait()
            # outfiledump += popen.stdout.read()
            with open(filename, 'wb') as infile:
                infile.write(outfiledump)
        section_flags += " --add-section " + value.newName + "=" + filename
        section_flags += " --change-section-address "+ value.newName + "=" + str(value.recVaddr)
        i-= 1
    args = ("/usr/bin/objcopy " + section_flags +" binary " + outfile )
    popen = subprocess.Popen(args,  shell=True)
    popen.wait()
    # outfiledump = popen.stdout.read()


def parseSections(brsList, inFile, whitelist, blacklist):
    args = ("/usr/bin/readelf " + "-SW " + inFile )
    popen = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    # recSecs = popen.stdout.read()
    recSecs = popen.communicate()[0]
    popen.wait()
    secLines =  recSecs.split('\n')[5:-5]  # strip readelf titles, legends
    #following regex parses readelf -S, collecting fields that are useful
    prog = re.compile("\s*\[.*\]\s*(?P<name>\S*)\s*(?P<type>\S*)\s*(?P<addr>\S{8}) \S* (?P<size>\S*) \S{2}\s+(?P<segtype>\S{1,3}).*")
    dataStart = None
    for line in secLines:
        match = prog.match(line)
        if not match:
            print "problem reading readelf -S"
            print line
            continue
        if match.group('name') not in blacklist:
            cur = BinrecSection(match.group('name'))
            cur.setRecVaddr(match.group('addr'))
            cur.setRecSize(match.group('size'))
            cur.setRecType(match.group('type'))
            # print match.group('name')
            # print match.group('addr')
            # print match.group('size')
            # print match.group('segtype')
            # first WA section is first section in data segment, all following sections are data
            # assumes first A section is always .text, if not true, constructsomething like this for that segment as well
            if ( dataStart is None ) and ('WA' in match.group('segtype')  ) and ('NOBITS' != match.group('type')) :
                # print match.group('name')
                cur.dataStart = True
                dataStart = cur.origName
                # cur.doPad = True
                dataNotFound = False
            brsList.append(cur)
            if match.group('name') not in whitelist:
                print "non-default section found: "+ match.group('name')
    return dataStart

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print 'python %s OUTFILE RECFILE no_link_lift_flag' % sys.argv[0] #noqa
        sys.exit(1)

    recfile = sys.argv[2]
    outfile = sys.argv[1]

    inNLL = sys.argv[3]
    no_link_lift = False
    if inNLL != "0" :
        no_link_lift = True

    # white/blacklist sections to copy from rec to out, any unspecified will be detected and copied
    sectionsForDynamicLinking = ['.dynsym', '.dynstr', '.gnu.hash', '.gnu.version', \
          '.gnu.version_r', '.rel.dyn', '.rel.plt', '.init', \
          '.plt', '.fini', '.init_array', '.fini_array', \
          '.got', '.got.plt' ]
    whitelist = ['.data', '.rodata', '.text','.bss' ]
    #use jcr as test
    blacklist = ['.interp', '.note.ABI-tag', '.note.gnu.build-id', '.dynamic', '.tm_clone_table',\
                 '.comment', '.note/gnu.gold-version', '.symtab', '.strtab', '.shstrtab',\
                 '.debug_info', '.debug_aranges', '.debug_line', '.debug_str', '.debug_loc', '.debug_ranges',\
                 '.debug_abbrev', \
                 '.eh_frame','.eh_frame_hdr','.note.gnu.gold-version', '.jcr']
    # TODO copy symtab/strtab for debug info? no WA flags, link and info fix too
    if no_link_lift :
        blacklist += sectionsForDynamicLinking
    else:
        whitelist +=  sectionsForDynamicLinking

    binrecSectionList = []  # could make this ordereddict
    dataStart = None
    dataStart = parseSections(binrecSectionList, recfile, whitelist, blacklist)
    copySections(binrecSectionList, outfile, recfile)
    # debug print
    # args = ("/bin/cp " + outfile +" testf")
    # popen = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    # popen.wait()


    doZwoelf(binrecSectionList, outfile, recfile,  dataStart, no_link_lift )

