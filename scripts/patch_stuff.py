#hacking up the output binary, only one use right now
#1) the first word in .got.plt needs to be the address of the .dynamic section

import binascii
import pickle
import sys
import os
import subprocess
import re

def hexPtrFromIntStr(inVal):
    temp = hex(int(inVal,10))
    #byte reversed
    while len(temp) < 10: #add back '0's elided by 0x notatation
        temp = temp[0:2]+"0"+temp[2:]
        return temp[8:10]+" "+temp[6:8]+" "+temp[4:6]+" "+temp[2:4]


# assuming 8 chars hex val
def byteReverse(inVal):
    return inVal[6:8]+inVal[4:6]+inVal[2:4]+inVal[0:2]


def zext(input, desiredLen):
    while len(input) < desiredLen:
        input = '0'+input
    return input


def hexFromIntStrNoSpace(inVal):
    temp = hex(int(inVal,10))
    return temp[2:]


#int = ofa(str,str,str)
def offsetFromAddr(base, fo, vaddr, intExtraOffset):
    #print base, fo, vaddr
    diff = int(vaddr, 16) - int(base, 16)
    return int(fo, 16) + diff + intExtraOffset


def relCall(src, dest):
    return hex(int(dest, 16) - int(src, 16) + 5)



def patchInit(outfiledump, recfiledump, targetBin):
    setupPatch = re.search(".*<\.new\.text>.*0x(\w*)\):",outfiledump)
    base = "9000000"
    boffset = setupPatch.group(1)
    destAddr = re.search("(\w{7}).*push.*0xdeadbeef", outfiledump).group(1)
    # print destAddr
    #outCallToInit = re.search("(\w{8}):.*call\s*"+recInitAddr[1:], outfiledump)
    # outCallToInit = re.search("(\w{8}):.*call\s*"+recInitAddr[1:]+".*File Offset: 0x(\w*)", outfiledump)
    #outCallToInitVaddr = outCallToInit.group(1)
    destFirstByte = offsetFromAddr(base, boffset, destAddr,1)
    # print destFirstByte
    outInitAddr = re.search("(\w{8}) <_init> ", outfiledump).group(1) #does this depend on debug info
    patch = byteReverse(outInitAddr)
    doPatch(targetBin, destFirstByte, patch)

def patchGOTPLTent0(outfiledump, recfiledump, targetBin):
    setupPatch = re.search(".*<\.new\.got\.plt>.*0x(\w*)\):",outfiledump)
    foffset = setupPatch.group(1)
    destFirstByte = int(foffset, 16)
    # print destFirstByte
    outInitAddr = re.search("(\w{8}) <_DYNAMIC> ", outfiledump).group(1) #does this depend on debug info
    patch = byteReverse(outInitAddr)
    doPatch(targetBin, destFirstByte, patch)


def doPatch(targetBin, destFirstByte, patch):
    with open(targetBin, 'r+b') as fh: # apply patch1
        fh.seek(destFirstByte)
        fh.write(binascii.a2b_hex(patch))


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print 'python %s outfile recfile ' % sys.argv[0] #noqa
        sys.exit(1)
    no_link_lift = sys.argv[3]

    args = ("/usr/bin/objdump", "-DF", sys.argv[1])
    popen = subprocess.Popen(args, stdout=subprocess.PIPE)
    outfiledump = popen.stdout.read()
    args = ("/usr/bin/objdump", "-DF", sys.argv[2])
    popen = subprocess.Popen(args, stdout=subprocess.PIPE)
    recfiledump = popen.stdout.read()

    if no_link_lift != "0":
        print "initpatch"
        patchInit(outfiledump, recfiledump, sys.argv[1])
    else :
        #the first word in .got.plt needs to be the address of the .dynamic section
        patchGOTPLTent0(outfiledump, recfiledump, sys.argv[1])


