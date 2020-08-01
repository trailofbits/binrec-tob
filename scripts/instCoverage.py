import sys
import os
import subprocess
import re


def toKeep(regex, x):
    if regex.match(x) is not None:
        return True

def doOne(irFile, binFile):
    uniquePCs = set()

    if not os.path.isfile(irFile):
        return -1, -1, -1.0
    with open(irFile, 'r') as rec:
        lines = rec.readlines()
    pat = re.compile('.*store i32 (\d+), i32\* @PC.*')
    for line in lines:
        # print line
        match = pat.match(line)
        if (match and match.groups()[0] not in uniquePCs):
            uniquePCs.add(match.groups()[0])
    recPCs = len(uniquePCs)

    args = ("/usr/bin/objdump", "-DF", binFile)
    popen = subprocess.Popen(args, stdout=subprocess.PIPE)
    outfiledump = popen.stdout.read()
    # include plt?
    # match = re.search('Disassembly of section \.text:(.*)Disas', outfiledump, re.DOTALL)
    match = re.search('Disassembly of section \.text:(.*)Disassembly of section \.fini', outfiledump, re.DOTALL)
    if match is not None:
        # print match.start() #noqa
        textSection = match.groups()[0]
    else:
        print "Couldn't find text section"
        exit(-1)
    trimEmpty = textSection.splitlines()[2:-2]
    startWithAddr = re.compile(' [0-9a-f]+:.*')
    trimEmpty[:] = [x for x in trimEmpty if toKeep(startWithAddr,x)] #remove label lines
    removeNOP1 = re.compile('.*66 90.*')
    trimEmpty[:] = [x for x in trimEmpty if not toKeep(removeNOP1,x)]
    removeNOP2 = re.compile('.*nop.*')
    trimEmpty[:] = [x for x in trimEmpty if not toKeep(removeNOP2,x)]
    origPCs = len(trimEmpty) -259 # about 250inst in the compiler added libc functions

    return recPCs, origPCs, float(recPCs)/origPCs

if __name__ == '__main__':
    # if len(sys.argv) < 3:
        # print 'python %s file.ll binary' % sys.argv[0]
        # sys.exit(1)

    with open('coverage_report','w') as results:
        # for dirr in next(os.walk('.'))[1]: #python3
        for dirr in [name for name in os.listdir('.')
                if os.path.isdir(os.path.join('.', name))]:
            if dirr[0] == '.':
                continue
            result = doOne(os.path.join(dirr, 'lif.ll'),os.path.join(dirr,'binary'))
            if result[0] is -1 :
                continue
            results.write('label: '+dirr+'    recPCs: '+str(result[0])+'  origPCs: '+str(result[1])+'    ratio: '+ str(result[2])+'\n' )

    # print  'recPCs: '+str(recPCs)+'  origPCs: '+str(origPCs)+'    ratio: '+ str(float(recPCs)/origPCs)
