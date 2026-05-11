#!/usr/bin/env python3
"""自动把常见的邻接矩阵/空格分隔矩阵转换为仓库的拓扑输入格式。
用法: convert_to_topo.py <infile> <outfile>
如果输入已经是仓库格式（第一行含逗号或仅一个整数 N），则会简单拷贝/normalize。
输出格式:
- 如果图是 d-正则: 第一行写 `N,d`，接下来 N 行每行写 d 个逗号分隔的邻居 id
- 否则: 第一行只写 `N`，接下来 N 行写可变长度的逗号分隔邻居列表
"""
import sys
import re

def is_binary_tokens(tokens):
    return all(t in ('0','1','0.0','1.0') for t in tokens)


def read_lines(path):
    with open(path,'r') as f:
        raw = f.read()
    # Remove common markdown fences accidentally included
    raw = raw.replace('```', '\n')
    lines = [ln.strip() for ln in raw.splitlines() if ln.strip()!='']
    return lines


def parse_input(lines):
    # If first non-empty line contains a comma and starts with an integer, assume already correct format
    first = lines[0]
    if ',' in first:
        parts = [p.strip() for p in first.split(',')]
        if len(parts)>0 and re.match(r'^\d+$', parts[0]):
            # assume already correct format -> return as-is
            return 'already_topo', lines
    # If first line is a single integer N, assume topo header without degree
    if re.match(r'^\d+$', first):
        return 'already_topo', lines
    # Otherwise, try to parse as adjacency matrix / space-separated rows
    matrix = []
    for ln in lines:
        # split by spaces or commas
        if ',' in ln:
            toks = [t.strip() for t in ln.split(',') if t.strip()!='']
        else:
            toks = [t.strip() for t in ln.split() if t.strip()!='']
        if len(toks)==0:
            continue
        matrix.append(toks)
    # If matrix tokens are binary (0/1) and square -> adjacency matrix
    if len(matrix)>0 and all(len(row)==len(matrix) for row in matrix) and all(is_binary_tokens(row) for row in matrix):
        return 'adj_matrix', matrix
    # Fallback: maybe adjacency lists where each line contains neighbor ids
    # try parse as int lists
    lists = []
    ok=True
    for ln in lines:
        toks = [t.strip() for t in re.split('[,\s]+', ln) if t.strip()!='']
        try:
            ints = [int(x) for x in toks]
        except Exception:
            ok=False
            break
        lists.append(ints)
    if ok:
        return 'adj_list', lists
    return 'unknown', lines


def write_topo_from_matrix(matrix, outpath):
    N = len(matrix)
    neigh = []
    degs = []
    for i,row in enumerate(matrix):
        # treat tokens '1' as edge
        nbrs = [j for j,val in enumerate(row) if val in ('1','1.0')]
        neigh.append(nbrs)
        degs.append(len(nbrs))
    # check regularity
    min_deg = min(degs)
    max_deg = max(degs)
    regular = (min_deg==max_deg)
    with open(outpath,'w') as f:
        if regular:
            f.write(f"{N},{max_deg}\n")
        else:
            f.write(f"{N}\n")
        for row in neigh:
            f.write(','.join(str(x) for x in row) + '\n')
    return N, regular, min_deg, max_deg


def write_topo_from_lists(lists, outpath):
    N = len(lists)
    degs = [len(l) for l in lists]
    min_deg = min(degs)
    max_deg = max(degs)
    regular = (min_deg==max_deg)
    with open(outpath,'w') as f:
        if regular:
            f.write(f"{N},{max_deg}\n")
        else:
            f.write(f"{N}\n")
        for l in lists:
            f.write(','.join(str(x) for x in l) + '\n')
    return N, regular, min_deg, max_deg


def main():
    if len(sys.argv)<3:
        print('Usage: convert_to_topo.py <infile> <outfile>')
        sys.exit(2)
    infile=sys.argv[1]
    outfile=sys.argv[2]
    lines = read_lines(infile)
    typ, data = parse_input(lines)
    if typ=='already_topo':
        # normalize: just copy
        with open(outfile,'w') as f:
            for ln in data:
                f.write(ln.strip()+"\n")
        print(f'Input looks already in topo format. Copied to {outfile}')
        return
    elif typ=='adj_matrix':
        N, regular, mind, maxd = write_topo_from_matrix(data, outfile)
        print(f'Converted adjacency-matrix -> topo. N={N}, regular={regular}, min_deg={mind}, max_deg={maxd}, out={outfile}')
        return
    elif typ=='adj_list':
        N, regular, mind, maxd = write_topo_from_lists(data, outfile)
        print(f'Converted adjacency-list -> topo. N={N}, regular={regular}, min_deg={mind}, max_deg={maxd}, out={outfile}')
        return
    else:
        print('Unknown input format; cannot convert automatically. First lines:')
        for ln in lines[:10]:
            print('> ', ln)
        sys.exit(1)

if __name__=='__main__':
    main()
