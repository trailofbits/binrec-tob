#!/usr/bin/env python
import argparse
import fileinput

def unpack(line):
    try:
        def value(part):
            k, v = part.split('=', 1)
            return k, int(v, 16)

        return map(value, line.split())
    except ValueError:
        pass

def pack(state):
    return ' '.join('%s=%-8x' % pair for pair in state)

def get(state, needle):
    for k, v in state:
        if k == needle:
            return v

    raise KeyError('key "%s" not found in state %s' % (needle, state))

def process(state, transform, *args):
    return filter(None, (transform(k, v, *args) for k, v in state))

def normalize(k, v, initial_stack_offset):
    if k in ('ebp', 'esp'):
        v += initial_stack_offset

    return k, v

def prune(k, v, prune_fields):
    if k not in prune_fields:
        return k, v

if __name__ == '__main__':
    fields = 'pc eax ebx ecx edx ebp esp esi edi cc_src cc_dst cc_op df mflags'.split()

    parser = argparse.ArgumentParser(description='Process a log file.')
    parser.add_argument('-n', '--normalize-stack', action='store_true',
            help='set initial esp to 0xffffffff')
    parser.add_argument('-r', '--remove', action='append', default=[],
            choices=fields,
            help='fields to remove')
    parser.add_argument('-k', '--keep', action='append', default=[],
            choices=fields,
            help='fields to keep')
    parser.add_argument('logfiles', nargs='*',
            help='optional input file(s), defaults to stdin')
    args = parser.parse_args()

    initial_stack_offset = None

    for line in fileinput.input(args.logfiles):
        state = unpack(line)

        if state:
            if args.normalize_stack:
                if initial_stack_offset is None:
                    initial_stack_offset = 0xffffffff - get(state, 'esp')

                state = process(state, normalize, initial_stack_offset)

            if args.remove:
                state = process(state, prune, args.remove)
            elif args.keep:
                state = process(state, prune,
                        [f for f in fields if f not in args.keep])

            print pack(state)
        else:
            print line.rstrip()
