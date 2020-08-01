#!/usr/bin/env python
import sys
from cfg import read_log, write_log, flatten_graph, build_graph, visit_graph, \
        end_states


def reset_preds(graph):
    for cur in visit_graph(graph):
        cur.preds = []

    for cur in visit_graph(graph):
        for succ in cur.succs:
            succ.preds.append(cur)


def mimimize_end_states(graph):
    def groups(nodes):
        indexed = {}

        for node in nodes:
            indexed.setdefault(node.addr, []).append(node)

        return [v for v in indexed.itervalues() if len(v) > 1]

    merge_sets = groups(end_states(graph))

    while merge_sets:
        new_sets = []

        for nodes in merge_sets:
            base = nodes[0]

            for obsolete in nodes[1:]:
                for pred in obsolete.preds:
                    pred.succs[pred.succs.index(obsolete)] = base
                    base.preds.append(pred)

                if obsolete.is_fork:
                    base.is_fork = True

            new_sets.extend(groups(base.preds))

        merge_sets = new_sets


def connect_chains(graph, keep_control_flow=False):
    worklist = [graph]
    keep = set()

    if keep_control_flow:
        allpreds = {}

        for cur in visit_graph(graph):
            preds = (pred.ident() for pred in cur.preds)
            allpreds.setdefault(cur.ident(), set()).update(preds)

        for cur in visit_graph(graph):
            if cur.is_fork or len(allpreds[(cur.addr, cur.state)]) > 1:
                keep.add(cur)

    while worklist:
        cur = worklist.pop(0)

        while len(cur.succs) == 1 == len(cur.succs[0].preds) and \
                cur.succs[0] not in keep:
            other = cur.succs[0]
            assert other.preds[0] is cur

            # replace predecessor references in successors
            for succ in other.succs:
                succ.preds[succ.preds.index(other)] = cur

            if other.is_fork:
                cur.is_fork = True

            cur.succs = other.succs
            del other

        worklist.extend(cur.succs)


if __name__ == '__main__':
    keep_control_flow = '-k' in sys.argv[1:]
    graph = build_graph(read_log(sys.stdin))
    mimimize_end_states(graph)
    reset_preds(graph)
    connect_chains(graph, keep_control_flow)
    write_log(sys.stdout, flatten_graph(graph))
