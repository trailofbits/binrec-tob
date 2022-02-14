#!/usr/bin/env python
import sys
import os
import subprocess
import re

if __name__ == '__main__':
    # frontend
    # If stripped binary, use nucleus
    # use s2e?
    # If unstripped, use objdump
    if len(sys.argv) < 2:
        print 'python %s BINARY ' % sys.argv[0]
        sys.exit(1)

    args = ("/usr/bin/objdump", "-D", sys.argv[1])
    popen = subprocess.Popen(args, stdout=subprocess.PIPE)
    output = popen.stdout.read()
    output = output.splitlines()


    startline, endline = 0,len(output)
    startline = output.index("Disassembly of section .text:")
    for index, line in enumerate(output[startline+1:]):
        if "Disassembly of section"  in line:
            endline = index
            break
    textSection = output[startline+1:startline+1+endline]
    print "start of text section = ", startline
    print "end of text section = ", startline+endline

    functionNameRegex = re.compile('<[^>]+>:')
    nameFirstRet = [] #funcName,addr of first inst, addr of ret
    for ind, item in enumerate(textSection):
        #print item
        m = functionNameRegex.search(item)
        if m:
         #print m.group()
         for instruction in textSection[ind:]:
            #print instruction
            if "ret" in instruction:
                temp = (m.group(0)[1:-2],textSection[ind+1].partition(':')[0],instruction.partition(':')[0])
                #temp = (textSection[ind+1].partition(':')[0][1:],instruction.partition(':')[0][1:])
                nameFirstRet.append(temp)
                #print temp
                break

    #backend
    outFileName = "%s.functions" % sys.argv[1]
    #print outFileName
    f = open(outFileName,"w")
    for function in nameFirstRet:
        if "main" in function[0]:
            continue
        f.write(function[1]+'\n')
        f.write(function[2]+'\n')
        #f.write(function[2]+'\n')
    f.close()



#int origCallTargetAddress; //32 bit ptr
#int origRetAddress;
