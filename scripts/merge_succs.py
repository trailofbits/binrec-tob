#!/usr/bin/env python
import fileinput
import sys
import binascii
import struct

def readFileToDict(f_path, succ_dict, new_edges=None):
    f = open(f_path, "rb")
    try:
        #sys.stdout.write("------------------\n")
        succ_bb = f.read(4)
        curr_bb = f.read(4)
        while curr_bb != "" and succ_bb != "":
            # Do stuff with byte.
            #sys.stdout.write(binascii.hexlify(succ_bb))
            #sys.stdout.write(' ')
            #sys.stdout.write(binascii.hexlify(curr_bb))
            #sys.stdout.write('\n')
            #print(binascii.hexlify(succ_bb))
            #print(binascii.hexlify(curr_bb))
            if curr_bb not in succ_dict:
                succ_dict[curr_bb] = set()
            #To see how many new edges we add
            if succ_bb not in succ_dict[curr_bb]:
                if new_edges is not None:
                    if curr_bb not in new_edges:
                        new_edges[curr_bb] = set()
                    new_edges[curr_bb].add(succ_bb)
            succ_dict[curr_bb].add(succ_bb)
            succ_bb = f.read(4)
            curr_bb = f.read(4)
    finally:
        #sys.stdout.write("------------------\n")
        f.close()

def writeDictToFile(f_path, succ_dict):
    f = open(f_path, "wb")
    try:
        for k, vSet in succ_dict.iteritems():
            for v in vSet:
                #print(binascii.hexlify(v))
                #print(binascii.hexlify(k))
                f.write(v)
                f.write(k)
    finally:
        f.close()

def debug(succ_dict):
    #print("Debug")
    for k, vSet in succ_dict.iteritems():
        #sys.stdout.write('\n')
        for v in vSet:
            sys.stdout.write(binascii.hexlify(v))
#            sys.stdout.write(struct.unpack("<H", binascii.hexlify(v)))
            sys.stdout.write(' ')
            sys.stdout.write(binascii.hexlify(k))
#            sys.stdout.write(struct.unpack("<H", binascii.hexlify(k)))
            sys.stdout.write('\n')

if __name__ == '__main__':
    
    succ_dict = {}
    new_edges = {}
    readFileToDict(sys.argv[1], succ_dict)
    readFileToDict(sys.argv[2], succ_dict, new_edges)

    print("---New Edges---")
    debug(new_edges)
    print("---------------")

    writeDictToFile(sys.argv[1], succ_dict)

