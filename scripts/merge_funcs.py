#!/usr/bin/env python
import fileinput
import sys
import binascii
import struct

def readFileToDict(f_path, set1, new_bbs=None):
    f = open(f_path, "rb")
    try:
        #sys.stdout.write("------------------\n")
        curr_bb = f.read(4)
        while curr_bb != "":
            # Do stuff with byte.
            #sys.stdout.write(binascii.hexlify(succ_bb))
            #sys.stdout.write(' ')
            #sys.stdout.write(binascii.hexlify(curr_bb))
            #sys.stdout.write('\n')
            #print(binascii.hexlify(succ_bb))
            #print(binascii.hexlify(curr_bb))
            if curr_bb not in set1:
                if new_bbs is not None:
                    new_bbs.add(curr_bb)
                set1.add(curr_bb)
            curr_bb = f.read(4)
    finally:
        #sys.stdout.write("------------------\n")
        f.close()

def writeDictToFile(f_path, set1):
    f = open(f_path, "wb")
    try:
        for k in set1:
                #print(binascii.hexlify(v))
                #print(binascii.hexlify(k))
            f.write(k)
    finally:
        f.close()

def debug(bb_set):
    #print("Debug")
    for k in bb_set:
        #sys.stdout.write('\n')
#            sys.stdout.write(struct.unpack("<H", binascii.hexlify(v)))
            sys.stdout.write(binascii.hexlify(k))
#            sys.stdout.write(struct.unpack("<H", binascii.hexlify(k)))
            sys.stdout.write('\n')

if __name__ == '__main__':
    
    set1 = set()
    new_bbs = set()
    readFileToDict(sys.argv[1], set1)
    readFileToDict(sys.argv[2], set1, new_bbs)

    print("---New func bbs---")
    debug(new_bbs)
    print("---------------")

    writeDictToFile(sys.argv[1], set1)

