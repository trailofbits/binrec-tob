#!/usr/bin/env python
import sys
from cfg import read_log, write_log

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print >>sys.stderr, "Usage: python %s LOGFILES..." % sys.argv[0]
        sys.exit(1)

    for logfile in sys.argv[1:]:
        with open(logfile, 'r') as f:
            write_log(sys.stdout, read_log(f))
