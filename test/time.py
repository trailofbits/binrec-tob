#!/usr/bin/env python
import sys

if __name__ == '__main__':
    start_sec, start_usec = map(int, sys.stdin.readline().split())
    end_sec, end_usec = map(int, sys.stdin.readline().split())

    print (end_sec + (end_usec / 1000000.0)) - \
        (start_sec + (start_usec / 1000000.0))
