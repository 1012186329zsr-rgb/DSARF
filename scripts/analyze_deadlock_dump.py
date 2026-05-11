#!/usr/bin/env python3
import sys
import re
from collections import defaultdict, deque

def read_topo(path):
    with open(path,'r') as f:
        hdr = f.readline().strip()
        n,d = map(int,hdr.split(','))
        adj = []
        for _ in range(n):
            line = f.readline().strip()
            if not line:
                adj.append([])
            else:
                adj.append([int(x) for x in line.split(',')])
    return adj

def parse_dump(path):
    # Returns dict: channels[(node,port,vc)] = {size:int,credit:int,buffer_o:int}
    channels = {}
    node = None
    port = None
    vc = None
    with open(path,'r') as f:
        for line in f:
            line=line.rstrip('\n')
            m = re.match(r'^Node (\d+):', line)
            if m:
                node = int(m.group(1))
                continue
            m = re.match(r'^  port (\d+):', line)
            if m:
                port = int(m.group(1))
                continue
            m = re.match(r'^    VC (\d+): size=(\d+) start=(\d+) end=(\d+) credit=(\d+) buffer_o.dst=([-0-9]+)', line)
            if m:
                vc = int(m.group(1))
                size = int(m.group(2))
                credit = int(m.group(5))
                buffer_o = int(m.group(6))
                channels[(node,port,vc)] = {'size':size,'credit':credit,'buffer_o':buffer_o}
                continue
            # ignore other lines
    return channels

def build_wait_graph(adj, channels):
    # For each channel C=(i,p,vc) that has size>0 and credit==0, add edge to neighbor's channel it would send to
    edges = defaultdict(set)
    n = len(adj)
    for (i,p,vc),info in channels.items():
        if info['size']>0 and info['credit']==0:
            # neighbor
            if p >= len(adj[i]):
                continue
            nb = adj[i][p]
            # find remote port q where adj[nb][q] == i
            q = None
            for idx, val in enumerate(adj[nb]):
                if val == i:
                    q = idx
                    break
            if q is None:
                continue
            edges[(i,p,vc)].add((nb,q,vc))
    return edges

def find_cycles(edges):
    # simple DFS to find cycles up to reasonable length
    cycles = []
    visited = set()
    def dfs(path, node, stack):
        if node in stack:
            idx = stack.index(node)
            cycles.append(stack[idx:]+[node])
            return
        stack.append(node)
        for nb in edges.get(node,[]):
            dfs(path, nb, stack)
        stack.pop()

    for node in edges.keys():
        dfs([], node, [])
    # deduplicate cycles (normalize)
    norm = set()
    uniq = []
    for c in cycles:
        # represent as tuple of nodes starting from smallest repr
        seq = tuple(c)
        # rotate to smallest
        rots = [tuple(seq[i:]+seq[:i]) for i in range(len(seq)-1)]
        key = min(rots)
        if key not in norm:
            norm.add(key)
            uniq.append(seq)
    return uniq

def main():
    if len(sys.argv) < 3:
        print('Usage: analyze_deadlock_dump.py <topo.txt> <deadlock_dump.txt>')
        sys.exit(2)
    topo = sys.argv[1]
    dump = sys.argv[2]
    adj = read_topo(topo)
    channels = parse_dump(dump)
    edges = build_wait_graph(adj, channels)
    print('Total channels parsed:', len(channels))
    print('Total wait edges:', sum(len(v) for v in edges.values()))
    cycles = find_cycles(edges)
    if not cycles:
        print('No cycles found in wait-for graph.')
    else:
        print('Found', len(cycles), 'cycles:')
        for idx,c in enumerate(cycles):
            print('Cycle',idx+1,':')
            for node in c:
                i,p,vc = node
                info = channels.get(node, {})
                print(f'  Node {i} port {p} VC {vc} size={info.get("size")} credit={info.get("credit")}')

if __name__=='__main__':
    main()
