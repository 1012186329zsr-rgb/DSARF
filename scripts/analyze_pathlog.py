#!/usr/bin/env python3
"""Analyze a path-log file (format produced by the simulator).
Outputs:
 - CSV with per-node totals and per-direction counts
 - summary printed and saved
 - heatmap (via plot_path_log.py) saved to supplied output path
Usage:
  python3 scripts/analyze_pathlog.py <path_log.txt> <out_prefix> [width]
Example:
  python3 scripts/analyze_pathlog.py temp/ramanujan_pathlog_seed0.txt results/ramanujan/seed0 33
"""
import sys
import os
import numpy as np
import csv
from math import sqrt


def read_pathlog(p):
    with open(p,'r') as f:
        header = f.readline().strip()
        parts = header.split(',')
        N = int(parts[0])
        deg = int(parts[1]) if len(parts)>1 else None
        data = []
        for line in f:
            vals = [int(x) for x in line.strip().split(',') if x!='']
            data.append(vals)
    arr = np.array(data, dtype=int)
    return N, deg, arr


def gini(array):
    # Gini coefficient for 1D array
    arr = np.array(array, dtype=float)
    if arr.size==0:
        return 0.0
    if np.all(arr==0):
        return 0.0
    arr = arr.flatten()
    arr = arr[arr>=0]
    n = arr.size
    sorted_arr = np.sort(arr)
    cum = np.cumsum(sorted_arr)
    g = (2.0 * np.sum((np.arange(1, n+1) * sorted_arr))) / (n * np.sum(sorted_arr)) - (n+1)/n
    return float(g)


def main():
    if len(sys.argv) < 3:
        print('Usage: analyze_pathlog.py <path_log.txt> <out_prefix> [width]')
        sys.exit(2)
    path_log = sys.argv[1]
    out_prefix = sys.argv[2]
    width = int(sys.argv[3]) if len(sys.argv)>=4 else None

    N, deg, arr = read_pathlog(path_log)
    totals = arr.sum(axis=1)

    # If out_prefix looks like a directory (ends with os.sep or has no extension),
    # create that directory and write files inside it (pernode.csv, summary.txt, heatmap.png).
    # This keeps results/<case>/ files grouped instead of many files in one folder.
    out_dir = None
    base = os.path.basename(out_prefix)
    # treat as directory if ends with separator or basename has no extension
    if out_prefix.endswith(os.sep) or os.path.splitext(base)[1] == '':
        out_dir = out_prefix.rstrip(os.sep)
    else:
        # if user passed a path like results/ramanujan/seed0 (no extension) treat as dir
        if os.path.dirname(out_prefix) and os.path.splitext(base)[1] == '':
            out_dir = out_prefix
        else:
            # fallback: use parent dirname for files with prefix
            out_dir = os.path.dirname(out_prefix) or '.'

    os.makedirs(out_dir, exist_ok=True)

    # write CSV
    # if out_prefix was intended as a directory use standardized filenames
    if os.path.splitext(base)[1] == '':
        csv_path = os.path.join(out_dir, 'pernode.csv')
        summary_path = os.path.join(out_dir, 'summary.txt')
        heatmap_path = os.path.join(out_dir, 'heatmap.png')
    else:
        csv_path = out_prefix + '_pernode.csv'
    with open(csv_path, 'w', newline='') as csvf:
        w = csv.writer(csvf)
        header = ['node'] + [f'dir_{i}' for i in range(arr.shape[1])] + ['total']
        w.writerow(header)
        for i,row in enumerate(arr):
            w.writerow([i] + row.tolist() + [int(totals[i])])

    # summary
    mx = int(totals.max())
    mn = int(totals.min())
    mean = float(totals.mean())
    med = float(np.median(totals))
    std = float(totals.std())
    q1 = float(np.percentile(totals, 25))
    q3 = float(np.percentile(totals, 75))
    topk = np.argsort(-totals)[:10]
    topk_vals = totals[topk]
    g = gini(totals)

    if os.path.splitext(base)[1] == '':
        # summary_path already set above
        pass
    else:
        summary_path = out_prefix + '_summary.txt'
    with open(summary_path, 'w') as sf:
        sf.write(f'N={N}, deg={deg}\n')
        sf.write(f'max={mx}, min={mn}, mean={mean:.4f}, median={med:.4f}, std={std:.4f}, q1={q1:.4f}, q3={q3:.4f}\n')
        sf.write('top10_nodes,total\n')
        for n,v in zip(topk, topk_vals):
            sf.write(f'{n},{int(v)}\n')
        sf.write(f'Gini={g:.6f}\n')

    # print summary
    print(open(summary_path).read())

    # generate heatmap using existing script
    if os.path.splitext(base)[1] == '':
        # heatmap_path already set above
        pass
    else:
        heatmap_path = out_prefix + '_heatmap.png'
    import subprocess
    cmd = ['python3','scripts/plot_path_log.py', path_log, heatmap_path]
    if width:
        cmd.append(str(width))
    subprocess.check_call(cmd)
    print('Wrote CSV:', csv_path)
    print('Wrote summary:', summary_path)
    print('Wrote heatmap:', heatmap_path)

if __name__=='__main__':
    main()
