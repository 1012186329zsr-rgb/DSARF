#!/usr/bin/env python3
import sys
import math
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


def read_path_log(path):
    with open(path, 'r') as f:
        header = f.readline().strip()
        if not header:
            raise ValueError('Empty path-log file')
        parts = header.split(',')
        N = int(parts[0])
        deg = int(parts[1]) if len(parts) > 1 else None
        data = []
        for line in f:
            line = line.strip()
            if not line:
                continue
            vals = [int(x) for x in line.split(',')]
            data.append(vals)
    arr = np.array(data, dtype=int)
    if arr.shape[0] != N:
        raise ValueError(f'Number of data lines ({arr.shape[0]}) does not match header N ({N})')
    return N, deg, arr


def main():
    if len(sys.argv) < 3:
        print('Usage: plot_path_log.py <path_log.csv> <out_image.png> [width]')
        sys.exit(2)
    inpath = sys.argv[1]
    outimg = sys.argv[2]
    width = int(sys.argv[3]) if len(sys.argv) >= 4 else None

    N, deg, arr = read_path_log(inpath)
    totals = arr.sum(axis=1)

    # Try to infer a square layout
    if width is None:
        side = int(round(math.sqrt(N)))
    else:
        side = width
    if side * side == N:
        grid = totals.reshape((side, side))
        fig, ax = plt.subplots(figsize=(6,6))
        im = ax.imshow(grid, cmap='hot', interpolation='nearest')
        ax.set_title(f'Path usage heatmap ({N} nodes, deg={deg})')
        fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
        plt.tight_layout()
        plt.savefig(outimg, dpi=150)
        print(f'Saved 2D heatmap {outimg} ({side}x{side})')
    else:
        # fallback: plot 1D bar of totals
        fig, ax = plt.subplots(figsize=(10,4))
        ax.bar(np.arange(N), totals)
        ax.set_title(f'Path usage per node (N={N}, deg={deg})')
        ax.set_xlabel('node id')
        ax.set_ylabel('total path count')
        plt.tight_layout()
        plt.savefig(outimg, dpi=150)
        print(f'Saved 1D bar plot {outimg} (N={N})')

    print('stats: max=', int(totals.max()), 'mean=', float(totals.mean()))


if __name__ == '__main__':
    main()
