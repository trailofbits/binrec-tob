#!/usr/bin/env python
import sys


class Node(object):
    def __init__(self, nid):
        self.nid = nid
        self.succs = set()
        self.preds = set()

    def add_succ(self, node):
        self.succs.add(node)
        node.preds.add(self)

    def optimize(self, visited=None):
        if visited is None:
            visited = set()

        while len(self.succs) == 1:
            succ = tuple(self.succs)[0]

            if len(succ.preds) == 1:
                assert succ not in visited
                self.succs = succ.succs

                for grandchild in succ.succs:
                    grandchild.preds.remove(succ)
                    grandchild.preds.add(self)

                del succ
            else:
                break

        visited.add(self)

        for succ in self.succs:
            if succ not in visited:
                succ.optimize(visited)

    def traverse(self, visited=None):
        if visited is None:
            visited = set()
        elif self in visited:
            return

        yield self
        visited.add(self)

        for succ in self.succs:
            for node in succ.traverse(visited):
                yield node


if __name__ == '__main__':
    filenames = sys.argv[1:]
    optimize = False

    root = None
    nodes = {}

    if filenames[0] in ('-o', '--optimize'):
        optimize = True
        filenames = filenames[1:]

    for filename in filenames:
        if filename == '-':
            filename = '/dev/stdin'

        with open(filename) as f:
            nid = int(f.readline())
            if root is None:
                nodes[nid] = root = Node(nid)
            elif nid != root.nid:
                print >>sys.stderr, \
                        'different start vpc: %d and %d' % (root.nid, nid)
                sys.exit(1)

            prev = root

            for line in f:
                nid = int(line)
                cur = nodes.setdefault(nid, Node(nid))
                prev.add_succ(cur)
                prev = cur

    if optimize:
        root.optimize()

    print 'digraph {'
    print 'node [shape=circle, style=filled]'

    for node in root.traverse():
        for succ in node.succs:
            print node.nid, '->', succ.nid

    print '}'
