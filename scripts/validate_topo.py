#!/usr/bin/env python3
"""验证拓扑文件是否符合仓库输入要求：\n- 第一行: N,deg\n- 接着 N 行: 每行 degree 个逗号分隔的邻居 id\n- 检查节点 id 范围 [0, N-1], 无自环, 度一致, 连通性\n"""
import sys
from collections import deque

def read_topo(path):
    with open(path,'r') as f:
        header = f.readline().strip()
        if not header:
            raise SystemExit('Empty file')
        parts = header.split(',')
        if len(parts) < 1:
            raise SystemExit('Bad header')
        N = int(parts[0])
        deg = int(parts[1]) if len(parts)>1 else None
        neigh = []
        for i in range(N):
            line = f.readline()
            if line is None or line.strip()=='':
                raise SystemExit(f'Missing line for node {i}')
            vals = [int(x) for x in line.strip().split(',') if x!='']
            neigh.append(vals)
    return N, deg, neigh


def validate(path):
    N, deg, neigh = read_topo(path)
    print(f'Parsed N={N}, deg={deg}')
    # degree check
    for i,arr in enumerate(neigh):
        if deg is not None and len(arr)!=deg:
            print(f'Warning: node {i} has degree {len(arr)} != header deg {deg}')
    # id range and self-loop
    for i,arr in enumerate(neigh):
        for v in arr:
            if v<0 or v>=N:
                raise SystemExit(f'Node {i} has neighbor {v} out of range [0,{N-1}]')
            if v==i:
                raise SystemExit(f'Node {i} has self-loop')
    # connectivity (BFS)
    seen=[False]*N
    q=deque([0])
    seen[0]=True
    while q:
        u=q.popleft()
        for v in neigh[u]:
            if not seen[v]:
                seen[v]=True
                q.append(v)
    if not all(seen):
        missing=[i for i,s in enumerate(seen) if not s]
        print('Graph not connected. Unreachable nodes:', missing[:10])
    else:
        print('Graph is connected')
    print('Validation done')

if __name__=='__main__':
    if len(sys.argv)<2:
        print('Usage: validate_topo.py <topo.txt>')
        sys.exit(2)
    validate(sys.argv[1])
