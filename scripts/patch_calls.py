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


def offsetFromAddr(base, fo, vaddr):
    diff = int(vaddr, 16) - int(base, 16)
    return hex(int(fo, 16) + diff)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print 'python %s new_binary ' % sys.argv[0]
        sys.exit(1)
    #argv[1] is the new binary we create
    #here we parse the binary dump to find the locations of trampolines
    args = ("/usr/bin/objdump", "-DF", sys.argv[1])
    popen = subprocess.Popen(args, stdout=subprocess.PIPE)
    odump = popen.stdout.read()

#1 during trampoline creation, give each a symbol name
#    get the address of the symbol here in pythotn script by searching name
#    this sounds like relocation, so look at loader if need complicated stuff
#1 alternative, regex search for instructions looking like what we know jmp table should be
# run this after move_sections
# Trampolines = list of addresses of 1st instruction of trampolines
#objdump -DF and parse

    trampolines = []
    replaceList = []

    inFileName = "rfuncs"
    with open(inFileName) as f:
	for line in f:
	    replaceList.append(line.strip())

    #print replaceList
    for item in replaceList:
	#key is mov Num %eax, opcode seems to be b8
        key = "b8 "+hexPtrFromIntStr(item)
	#print key
        i1 = odump.find(key)
        kLineStart = odump.rfind("\n",i1-100,i1)
        #jLineStart = odump.rfind("\n",kLineStart-400,kLineStart)  # assuming line length less than 400
	#base = odump[jLineStart+2:jLineStart+9] #7 numbers
	test = odump[kLineStart+2:kLineStart+9] #7 numbers
	#print "key addr should be trampaddr + 5 "+ test
        #print "fuck "+
	base = hex(int(test,16)-2)[2:]
	#if len(base) == 8:
        print "this is addr of trampline " + base
	    #print "len8 taken"
	#    trampolines.append(base)
	#elif len(base) == 7:
	    #print "len7 taken"
	trampolines.append('0'+base)
	#else:
	    #raise error
	#    print "wrong length of address in finding trampoline"

    if( len(trampolines) != len(replaceList)):
	print 'mismatch in number of trampolines and funcs'
	sys.exit(1)


    for i,vaddr in enumerate(replaceList):
        target = trampolines[i] #target is 8chars long, the trampolien addr
	#print target
	formatTarget = byteReverse(target)
        formatTarget = formatTarget[0:2]+" "+formatTarget[2:6]+" "+formatTarget[6:8] #format addr for xxd
        #iOfVaddr = odump.find(hexFromIntStrNoSpace(vaddr))
	vaddrH = hexFromIntStrNoSpace(vaddr)
	prog = re.compile(" "+vaddrH+":")
	result = prog.search(odump)
	iOfVaddr = result.start()
        foStart = odump.rfind("File Offset:",0,iOfVaddr)
        foLineStart = odump.rfind("\n",foStart-100,foStart)
        base = odump[foLineStart+1:foLineStart+9]
        fo = odump[foStart+12:foStart+24].split("0x",1)[1].split(")",1)[0]
        soffset = offsetFromAddr(base,fo,vaddrH) #get file offset of inst to overwrite

#        print str(iOfVaddr)+"	"+str(foStart)+"        "+str(foLineStart)+"        "+base+"        "+fo+"        "+str(soffset)
	out_buff = zext(soffset[2:],7) + ": "+ "68" + formatTarget+"c3" #compose push addr ret
        #objdump and parse to hex
        #   % echo "0000037: 3574 68" | xxd -r - xxd.1
        args = ("echo \"" + out_buff + "\" | xxd -r - "+ sys.argv[1])
        print args
	popen = subprocess.Popen(args,shell=True, stdout=subprocess.PIPE)
        stdo = popen.stdout.read()
	print stdo
	#todo error handling



#alternative to xxd way to write
# with file('file.bin', 'r+b') as fh: # apply patch1 fh.seek(0xc0010) fh.write(patch1)
