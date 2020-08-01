#!/usr/bin/env python
import sys

if __name__ == '__main__':
    def readline():
        parts = sys.stdin.readline().split()
        assert len(parts) == 2
        return parts[0], float(parts[1])

    HZ = None

    label, utime = readline()
    assert label == 'utime'

    label, small = readline()
    assert label == 'small'

    label, n = readline()
    if label == 'HZ':
        HZ = n
        label, runs = readline()
    else:
        runs = n
    assert label == 'runs'

    if HZ is not None:
        runs *= HZ

    if utime < small:
        print 'Measured time too small to obtain meaningful results'
        print 'Please increase number of runs'
    else:
        ms = utime * 1000000.0 / runs
        dhrs = runs / utime;
        print 'Microseconds for one run through Dhrystone: %6.3f' % ms
        print 'Dhrystones per Second:                      %6.1f' % dhrs
