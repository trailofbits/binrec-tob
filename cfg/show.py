#!/usr/bin/env python
import sys
from cfg import read_log_pairs, write_pair, LOG_SEPARATOR, STATE_FORK, \
        STATE_SWITCH, STATE_RETURN

MAX_LINES = 10000

if __name__ == '__main__':
    for nlines, (start, end) in enumerate(read_log_pairs(sys.stdin)):
        if start == LOG_SEPARATOR:
            print >>sys.stderr, '---------------------------------------------'
        elif start == STATE_FORK:
            print >>sys.stderr, '-- state fork:', end
        elif start == STATE_SWITCH:
            print >>sys.stderr, '-- state switch:', end
        elif start == STATE_RETURN:
            print >>sys.stderr, '-- state return:', end
        else:
            print >>sys.stderr, '%08x - %08x' % (start, end)

        write_pair(sys.stdout, start, end)

        if nlines == MAX_LINES:
            print >>sys.stderr, '-- log broken off at %d lines' % MAX_LINES
            break
