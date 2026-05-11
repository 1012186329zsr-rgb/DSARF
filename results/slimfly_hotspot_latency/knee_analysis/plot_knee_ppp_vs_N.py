#!/usr/bin/env python3
import csv
from pathlib import Path
import matplotlib.pyplot as plt

base = Path(__file__).resolve().parent
csv_path = base / "injection_knee_summary_compact.csv"

ns = []
knee_95 = []
knee_geo = []

with csv_path.open("r", encoding="utf-8", newline="") as f:
    reader = csv.DictReader(f)
    for row in reader:
        n = int(row["N"])
        ns.append(n)
        knee_95.append(float(row["knee_ppp_95pct_throughput"]) if row["knee_ppp_95pct_throughput"] else None)
        knee_geo.append(float(row["knee_ppp_latency_geometry"]) if row["knee_ppp_latency_geometry"] else None)

x1 = [n for n, v in zip(ns, knee_95) if v is not None]
y1 = [v for v in knee_95 if v is not None]
x2 = [n for n, v in zip(ns, knee_geo) if v is not None]
y2 = [v for v in knee_geo if v is not None]

plt.figure(figsize=(8, 5))
if x1:
    plt.plot(x1, y1, marker="o", label="knee ppp (95% throughput)")
if x2:
    plt.plot(x2, y2, marker="s", label="knee ppp (latency geometry)")

plt.xlabel("N")
plt.ylabel("Injection rate ppp")
plt.title("Hotspot knee point vs topology size")
plt.grid(True, alpha=0.3)
plt.legend()
plt.tight_layout()

out_png = base / "knee_ppp_vs_N.png"
plt.savefig(out_png, dpi=150)
print(f"Saved {out_png}")
