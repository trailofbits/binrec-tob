#!/usr/bin/env python
import sys
import subprocess
import re


def parseSections(infile):
    args = ("/usr/bin/readelf " + "--dyn-syms " + infile)
    popen = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    recSecs = popen.communicate()[0]
    popen.wait()
    secLines =  recSecs.split('\n')[3:-1]  # strip titles, legends
    #following regex parses readelf --dyn-syms, collecting fields that are useful
    prog = re.compile("\s*\d*:\s(?P<addr>\w{8})\s*\w{1,3} (?P<type>\w{4,8})\s*\w*\s*\w*\s*\w* ((?P<name>[^@\s]*)|(?P<atname>\S*)@.*)")
    out = []
    for line in secLines:
        match = prog.match(line)
        if not match:
            print "problem reading readelf --dyn-syms, offending line follows" #noqa
            print line
            continue
        if match.group('addr') == '00000000':
            continue
        if match.group('type') == 'FUNC':
            # print "found func  "+match.group('addr')
            continue #we do not want to add global objects for function type symbols
        if match.group("atname"):
            out += [(match.group('addr'), match.group('atname'))]
        else:
            out += [(match.group('addr'), match.group('name'))]
    return out

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print 'python %s OUTFILE ' % sys.argv[0] #noqa
        sys.exit(1)

    outfile = sys.argv[1]
    data = parseSections("binary")
    # print data
    with open(sys.argv[1],'w') as outf:
        for tup in data:
            outf.write(tup[0]+" "+tup[1]+"\n")


