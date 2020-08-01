#!/usr/bin/env python
import sys
import struct


LOG_SEPARATOR = 0
STATE_FORK    = 1
STATE_SWITCH  = 2
STATE_RETURN  = 3

ADDR_FORMAT = '=L'
ADDR_WIDTH = struct.calcsize(ADDR_FORMAT)


def read_log(f):
    addr = f.read(ADDR_WIDTH)

    while addr:
        yield struct.unpack(ADDR_FORMAT, addr)[0]
        addr = f.read(ADDR_WIDTH)


def read_log_pairs(f):
    w = struct.calcsize('=LL')
    pair = f.read(w)

    while pair:
        yield struct.unpack('=LL', pair)
        pair = f.read(w)


def terminate(log):
    for addr in log:
        yield addr

    if addr != LOG_SEPARATOR:
        yield LOG_SEPARATOR


def read_logs(f):
    addrs = []

    for addr in read_log(f):
        if addr == LOG_SEPARATOR:
            if len(addrs) > 0:
                yield addrs
                addrs = []
        else:
            addrs.append(addr)

    if len(addrs) > 0:
        yield addrs


def write_addr(f, addr):
    f.write(struct.pack(ADDR_FORMAT, addr))


def write_pair(f, start, end):
    f.write(struct.pack('=LL', start, end))


def write_log(f, addrs):
    for addr in addrs:
        write_addr(f, addr)


class pc(object):
    def __init__(self, addr, state, end_addr=None, is_fork=False):
        self.addr = addr
        self.end_addr = end_addr
        self.state = state
        self.succs = []
        self.preds = []
        self.is_fork = is_fork

    def add_succ(self, succ):
        self.succs.append(succ)
        succ.preds.append(self)

    def __str__(self):
        return '%08x-%d-%d' % (self.addr, self.state, id(self))

    def __repr__(self):
        if self.end_addr is not None:
            return '%x-%x-%d' % (self.addr, self.end_addr, self.state)

        return '%x-%d' % (self.addr, self.state)

    def ident(self):
        return (self.addr, self.state)

    def nodeid(self):
        return 'x%08x_%d' % (self.addr, self.state)

    def is_state_switch(self):
        return len(set(succ.state for succ in self.succs)) > 1

    def contains(self, addr):
        assert self.end_addr is not None
        return self.addr <= addr < self.end_addr


def build_graph(trace):
    trace = list(trace)
    state = 0
    dummy = pc(None, 0)
    pcs = {0: dummy}
    nodes = {}
    i = 0

    while i < len(trace):
        if trace[i] == STATE_FORK:
            pcs[trace[i + 1]] = pcs[state]
            pcs[state].is_fork = True
            i += 2
        elif trace[i] == STATE_SWITCH:
            state = trace[i + 1]
            i += 2
        elif trace[i] == STATE_RETURN:
            ret = nodes[(trace[i + 2], trace[i + 1])]
            pcs[state].add_succ(ret)
            pcs[state] = ret
            i += 3
        else:
            end_addr = trace[i + 1]
            #cur = nodes.setdefault((trace[i], state), pc(trace[i], state, end_addr))
            nodes[(trace[i], state)] = cur = pc(trace[i], state, end_addr)
            pcs[state].add_succ(cur)
            pcs[state] = cur
            i += 2

    dummy.succs[0].preds = []
    return dummy.succs[0]


def flatten_graph(graph):
    worklist = [(0, graph)]
    state = 0
    new_state = 1
    node_states = {}

    while worklist:
        state, cur = worklist.pop(0)

        if state != 0:
            yield STATE_SWITCH
            yield state

        while cur is not None:
            node_states[cur] = state
            yield cur.addr
            yield cur.end_addr

            if len(cur.succs) > 0:
                # if there are multiple successors, generate new states
                if len(cur.succs) > 1 or cur.is_fork:
                    for succ in cur.succs[1:]:
                        yield STATE_FORK
                        yield new_state
                        worklist.append((new_state, succ))
                        new_state += 1

                    yield STATE_FORK
                    yield state

                # select first successor, which is int the same state by
                # default (otherwise see below)
                cur = cur.succs[0]

                # when switching back to another state, only log the first
                # address in that state and then terminate to prevent
                # additional edge counts in the resulting CFG plot
                next_state = node_states.get(cur, state)

                if next_state != state:
                    yield STATE_RETURN
                    yield next_state
                    yield cur.addr
                    cur = None
            else:
                cur = None


def visit_graph(graph):
    worklist = [graph]
    visited = set()

    while worklist:
        cur = worklist.pop(0)

        if cur in visited:
            # loop
            continue

        visited.add(cur)
        yield cur
        worklist.extend(cur.succs)


def end_states(graph):
    for cur in visit_graph(graph):
        if len(cur.succs) == 0:
            yield cur


def props(**kwargs):
    if len(kwargs) == 0:
        return ''

    args = ((k, v) for k, v in kwargs.iteritems() if v is not None)
    return '[' + ','.join('%s=%s' % p for p in args) + ']' if args else ''


if __name__ == '__main__':
    import sys
    label_nodes = '-l' in sys.argv[1:]

    print 'digraph cfg {'
    print '_entry [shape=plaintext, label="entry"]'
    print '_exit  [shape=plaintext, label="exit"]'
    print 'node [shape=circle, style=filled, label=""]'

    graph = build_graph(read_log(sys.stdin))
    worklist = [graph]
    visited = set()
    edges = {}

    print '_entry ->', graph.nodeid()

    while worklist:
        cur = worklist.pop(0)

        if cur in visited:
            continue

        visited.add(cur)

        if cur.is_state_switch():
            color = 'red'
        elif cur.is_fork:
            color = 'blue'
        else:
            color = None

        label = cur.addr if label_nodes else None
        print cur.nodeid(), props(fillcolor=color, label=label)

        for succ in cur.succs:
            ident = (cur.ident(), succ.ident())
            edges[ident] = edges.get(ident, 0) + 1

        if len(cur.succs) == 0:
            print cur.nodeid(), '-> _exit'

        worklist.extend(cur.succs)

    for (src, tgt), count in edges.iteritems():
        print '%s -> %s' % (pc(*src).nodeid(), pc(*tgt).nodeid()), \
                props(label=None if count == 1 else '"%dx"' % count)

    print '}'
