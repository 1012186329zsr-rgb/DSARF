#!/usr/bin/env python3
"""Compute per-(src,dst) path-set average/max hop length for a topology.

Topology format:
- First line: "N,degree" (comma/space separated)
- Next N lines: adjacency list of length=degree (comma/space separated)

Path-set definitions:
- mode=shortest:
	Path set = all shortest paths between (src,dst).
	Since all shortest paths have the same hop count, avg_len == max_len == dist.

- mode=upto (K<=3 supported):
	Path set = all simple paths from src to dst with length in [1..K].
	"Simple" here means no repeated nodes along the path.

Outputs a CSV with one row per ordered pair (src,dst), src!=dst.
Columns: src,dst,avg_len,max_len
Optionally include num_paths with --include-count.

Examples:
  python3 scripts/pathset_length_stats.py --topo temp/slimfly_N98_D11.txt --mode shortest --out results/N98_shortest.csv
  python3 scripts/pathset_length_stats.py --topo temp/slimfly_N98_D11.txt --mode upto --max-hops 3 --out results/N98_upto3.csv --include-count
"""

from __future__ import annotations

import argparse
import csv
import re
from collections import deque
from pathlib import Path
from typing import List, Tuple


def _split_ints(line: str) -> List[int]:
	parts = [p for p in re.split(r"[\s,]+", line.strip()) if p]
	return [int(p) for p in parts]


def read_topology(path: Path) -> Tuple[List[List[int]], int, int]:
	text = path.read_text(errors="ignore").strip().splitlines()
	if not text:
		raise ValueError(f"Empty topology file: {path}")
	header = _split_ints(text[0])
	if len(header) < 2:
		raise ValueError(f"Bad header (expected N,degree): {text[0]!r}")
	n, degree = header[0], header[1]
	if n <= 0 or degree < 0:
		raise ValueError(f"Invalid N/degree: N={n}, degree={degree}")
	if len(text) < 1 + n:
		raise ValueError(f"File has {len(text)-1} adjacency lines, expected {n}")

	neighbors: List[List[int]] = []
	for i in range(n):
		row = _split_ints(text[1 + i])
		if degree != 0 and len(row) < degree:
			raise ValueError(f"Node {i} adjacency length {len(row)} < degree {degree}")
		neighbors.append(row[:degree])

	return neighbors, n, degree


def all_pairs_shortest_dist(neighbors: List[List[int]]) -> List[List[int]]:
	n = len(neighbors)
	dist = [[-1] * n for _ in range(n)]
	for s in range(n):
		q = deque([s])
		dist[s][s] = 0
		while q:
			u = q.popleft()
			du = dist[s][u]
			for v in neighbors[u]:
				if v < 0 or v >= n:
					continue
				if dist[s][v] == -1:
					dist[s][v] = du + 1
					q.append(v)
	return dist


def compute_upto_k(neighbors: List[List[int]], k: int):
	if k < 1:
		raise ValueError("max_hops must be >=1")
	if k > 3:
		raise ValueError("mode=upto currently supports max_hops <= 3")

	n = len(neighbors)

	# counts_l[src][dst]
	c1 = [[0] * n for _ in range(n)]
	c2 = [[0] * n for _ in range(n)]
	c3 = [[0] * n for _ in range(n)]

	if k >= 1:
		for s in range(n):
			for t in neighbors[s]:
				if 0 <= t < n and t != s:
					c1[s][t] += 1

	if k >= 2:
		for s in range(n):
			for u in neighbors[s]:
				if not (0 <= u < n) or u == s:
					continue
				for t in neighbors[u]:
					if not (0 <= t < n) or t == s or t == u:
						continue
					c2[s][t] += 1

	if k >= 3:
		# s -> u -> v -> t
		for s in range(n):
			for u in neighbors[s]:
				if not (0 <= u < n) or u == s:
					continue
				for v in neighbors[u]:
					if not (0 <= v < n) or v == s or v == u:
						continue
					for t in neighbors[v]:
						if not (0 <= t < n) or t == s or t == u or t == v:
							continue
						c3[s][t] += 1

	# stats per pair
	avg = [[None] * n for _ in range(n)]
	mx = [[None] * n for _ in range(n)]
	tot = [[0] * n for _ in range(n)]

	for s in range(n):
		for t in range(n):
			if s == t:
				continue
			total = 0
			weighted = 0
			max_len = None
			if k >= 1 and c1[s][t]:
				total += c1[s][t]
				weighted += 1 * c1[s][t]
				max_len = 1
			if k >= 2 and c2[s][t]:
				total += c2[s][t]
				weighted += 2 * c2[s][t]
				max_len = 2
			if k >= 3 and c3[s][t]:
				total += c3[s][t]
				weighted += 3 * c3[s][t]
				max_len = 3
			if total > 0:
				avg[s][t] = weighted / total
				mx[s][t] = max_len
				tot[s][t] = total

	return avg, mx, tot


def main() -> int:
	ap = argparse.ArgumentParser()
	ap.add_argument("--topo", required=True, type=Path)
	ap.add_argument("--mode", choices=["shortest", "upto"], required=True)
	ap.add_argument("--max-hops", type=int, default=3)
	ap.add_argument("--out", required=True, type=Path)
	ap.add_argument("--include-count", action="store_true")
	args = ap.parse_args()

	neighbors, n, degree = read_topology(args.topo)

	rows = []
	if args.mode == "shortest":
		dist = all_pairs_shortest_dist(neighbors)
		for s in range(n):
			for t in range(n):
				if s == t:
					continue
				d = dist[s][t]
				if d < 0:
					continue
				row = {"src": s, "dst": t, "avg_len": float(d), "max_len": int(d)}
				if args.include_count:
					row["num_paths"] = ""  # unknown without path enumeration
				rows.append(row)
	else:
		avg, mx, tot = compute_upto_k(neighbors, args.max_hops)
		for s in range(n):
			for t in range(n):
				if s == t:
					continue
				if avg[s][t] is None:
					continue
				row = {"src": s, "dst": t, "avg_len": avg[s][t], "max_len": mx[s][t]}
				if args.include_count:
					row["num_paths"] = tot[s][t]
				rows.append(row)

	args.out.parent.mkdir(parents=True, exist_ok=True)
	fieldnames = ["src", "dst", "avg_len", "max_len"]
	if args.include_count:
		fieldnames.append("num_paths")
	with args.out.open("w", newline="") as f:
		w = csv.DictWriter(f, fieldnames=fieldnames)
		w.writeheader()
		w.writerows(rows)

	print(f"topo={args.topo} N={n} degree={degree} mode={args.mode} rows={len(rows)} out={args.out}")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())