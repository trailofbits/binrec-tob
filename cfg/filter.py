#!/usr/bin/env python
import sys
from cfg import read_log, write_log, LOG_SEPARATOR

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print >>sys.stderr, "Usage: python %s LBOUND UBOUND" % sys.argv[0]
        sys.exit(1)

    lbound, ubound = [int(h, 16) for h in sys.argv[1:3]]

    write_log(sys.stdout, (addr for addr in list(read_log(sys.stdin))
                           if lbound <= addr < ubound or addr == LOG_SEPARATOR))
