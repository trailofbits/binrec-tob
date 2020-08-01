#!/usr/bin/env python
import sys
from cfg import read_log, write_log

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print >>sys.stderr, "Usage: python %s START [ END ]" % sys.argv[0]
        sys.exit(1)

    start = int(sys.argv[1], 16)
    end = int(sys.argv[2], 16) if len(sys.argv) > 2 else None

    try:
        addrs = list(read_log(sys.stdin))
        left = addrs.index(start)
        right = len(addrs) if end is None else addrs.index(end, left) + 1
        write_log(sys.stdout, addrs[left:right])
    except ValueError as e:
        print >>sys.stderr, '%s: error: address %08x not found' % \
                            (sys.argv[0], int(e.args[0].split()[0]))
        sys.exit(1)
