#!/usr/bin/env python
import sys
from cfg import build_graph, read_log, visit_graph


def select_main(graph, entry, exits):
    in_main = False

    for node in visit_graph(graph):
        if node.addr == entry:
            in_main = True
            yield node.addr
        elif any(node.contains(exit) for exit in exits):
            yield node.addr
            node.succs = []
        elif in_main:
            yield node.addr


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print >>sys.stderr, "Usage: python %s MAIN_ENTRY MAIN_EXITS..." % \
                sys.argv[0]
        sys.exit(1)

    entry = int(sys.argv[1], 16)
    exits = [int(arg, 16) for arg in sys.argv[2:]]

    graph = build_graph(read_log(sys.stdin))

    for addr in sorted(set(select_main(graph, entry, exits))):
        if addr == entry:
            flag = ' e'
        elif addr in exits:
            flag = ' x'
        else:
            flag = ''

        print '%x%s' % (addr, flag)
