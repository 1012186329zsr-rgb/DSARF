#!/usr/bin/env python3
"""谱检查脚本：计算邻接矩阵的特征值并给出第二大特征值（用于判断 Ramanujan 上界）
注意：适用于中等规模（N<=2000），更大规模需要稀疏方法。
用法: check_ramanujan.py <topo.txt>
输出: N, degree, lambda_max, ramanujan_bound = 2*sqrt(d-1)
"""
import sys
import numpy as np
from math import sqrt

def read_topo(path):
    with open(path,'r') as f:
        header = f.readline().strip()
        parts = header.split(',')
        N=int(parts[0])
        deg=int(parts[1]) if len(parts)>1 else None
        neigh=[]
        for i in range(N):
            line=f.readline().strip()
            vals=[int(x) for x in line.split(',') if x!='']
            neigh.append(vals)
    return N,deg,neigh

def adjacency_matrix(N,neigh):
    A = np.zeros((N,N), dtype=float)
    for i,arr in enumerate(neigh):
        for v in arr:
            A[i,v]=1.0
    # assume undirected: symmetrize
    A = np.maximum(A, A.T)
    return A

if __name__=='__main__':
    if len(sys.argv)<2:
        print('Usage: check_ramanujan.py <topo.txt>')
        sys.exit(2)
    path=sys.argv[1]
    N,deg,neigh=read_topo(path)
    print(f'N={N}, deg={deg}')
    A=adjacency_matrix(N,neigh)
    vals = np.linalg.eigvals(A)
    vals = np.sort(np.real(vals))[::-1]
    lambda1 = vals[0]
    lambda2 = vals[1] if len(vals)>1 else 0
    bound = 2*sqrt(deg-1) if deg and deg>1 else None
    print('largest eigenvalues (real parts):', vals[:10])
    print('lambda1=', lambda1)
    print('lambda2=', lambda2)
    print('ramanujan bound 2*sqrt(d-1)=', bound)
    if bound is not None:
        print('lambda2 <= bound ? ', lambda2 <= bound)
    else:
        print('Cannot compute bound (deg missing)')
