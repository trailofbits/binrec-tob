#!/usr/bin/env python
import sys
from cfg import read_log, write_log, flatten_graph, build_graph, pc


def expand_pcs(addrs):
    addrs = iter(addrs)
    prev = head = pc(addrs.next())

    for addr in addrs:
        cur = pc(addr)
        prev.add_succ(cur)
        prev = cur

    return head


def merge(graph, trace):
    cur = pc(None, [graph])

    for i, addr in enumerate(trace):
        # traverse into the branch that starts with `addr`, of which there
        # should only be one (otherwise there would not be a branch)
        succ_found = False

        for succ in cur.succs:
            if succ.addr == addr:
                if succ_found:
                    raise ValueError('duplicate successor %08x -> %08x' %
                                     (cur.addr, addr))

                cur = succ
                succ_found = True

        # if no branch in the graph matches, create a new one and return
        if not succ_found:
            cur.add_succ(expand_pcs(trace[i:]))
            break


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print >>sys.stderr, "Usage: python %s LOGFILES..." % sys.argv[0]
        sys.exit(1)

    with open(sys.argv[1], 'r') as f:
        combined = build_graph(read_log(f))

    for logfile in sys.argv[2:]:
        with open(logfile, 'r') as f:
            merge(combined, list(read_log(f)))

    write_log(sys.stdout, flatten_graph(combined))
