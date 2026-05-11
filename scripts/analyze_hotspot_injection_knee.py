#!/usr/bin/env python3
import csv
import math
import os
import re
from typing import Dict, List, Optional


REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
HOTSPOT_DIR = os.path.join(REPO_ROOT, "results", "slimfly_hotspot_latency")
OUTPUT_CSV = os.path.join(HOTSPOT_DIR, "injection_knee_summary.csv")


def parse_topology_n(topology_name: str) -> int:
    match = re.search(r"_N(\d+)_", topology_name)
    return int(match.group(1)) if match else 0


def to_float(value: str) -> Optional[float]:
    try:
        if value is None or value == "":
            return None
        return float(value)
    except (ValueError, TypeError):
        return None


def load_curve(csv_path: str) -> List[Dict[str, float]]:
    points: List[Dict[str, float]] = []
    with open(csv_path, "r", encoding="utf-8", newline="") as file:
        reader = csv.DictReader(file)
        for row in reader:
            ppp = to_float(row.get("ppp", ""))
            offered = to_float(row.get("offered_traffic", ""))
            latency = to_float(row.get("avg_latency", ""))
            throughput = to_float(row.get("throughput", ""))
            if ppp is None or offered is None or latency is None or throughput is None:
                continue
            points.append(
                {
                    "ppp": ppp,
                    "offered": offered,
                    "latency": latency,
                    "throughput": throughput,
                }
            )
    points.sort(key=lambda point: point["offered"])
    return points


def find_first_throughput_95pct_point(points: List[Dict[str, float]]) -> Optional[Dict[str, float]]:
    if not points:
        return None
    peak = max(point["throughput"] for point in points)
    threshold = 0.95 * peak
    for point in points:
        if point["throughput"] >= threshold:
            return point
    return None


def point_line_distance(x: float, y: float, x1: float, y1: float, x2: float, y2: float) -> float:
    denom = math.hypot(y2 - y1, x2 - x1)
    if denom == 0:
        return 0.0
    numer = abs((y2 - y1) * x - (x2 - x1) * y + x2 * y1 - y2 * x1)
    return numer / denom


def find_knee_by_latency_geometry(points: List[Dict[str, float]]) -> Optional[Dict[str, float]]:
    if len(points) < 3:
        return None
    start = points[0]
    end = points[-1]
    best_point = None
    best_dist = -1.0
    for point in points[1:-1]:
        dist = point_line_distance(
            point["offered"],
            point["latency"],
            start["offered"],
            start["latency"],
            end["offered"],
            end["latency"],
        )
        if dist > best_dist:
            best_dist = dist
            best_point = point
    return best_point


def summarize_curve(topology_dir: str, csv_path: str) -> Dict[str, object]:
    points = load_curve(csv_path)
    topology = os.path.basename(topology_dir).replace("_hot20", "")
    n_value = parse_topology_n(topology)

    summary: Dict[str, object] = {
        "topology": topology,
        "N": n_value,
        "num_points": len(points),
        "offered_min": "",
        "offered_max": "",
        "peak_throughput": "",
        "offered_at_peak_throughput": "",
        "knee_ppp_95pct_throughput": "",
        "knee_offered_95pct_throughput": "",
        "knee_latency_95pct_throughput": "",
        "knee_ppp_latency_geometry": "",
        "knee_offered_latency_geometry": "",
        "knee_latency_geometry": "",
        "latency_growth_ratio_max_over_min": "",
        "throughput_change_max_over_min": "",
        "status": "insufficient_points",
    }

    if not points:
        summary["status"] = "no_data"
        return summary

    offered_min = points[0]["offered"]
    offered_max = points[-1]["offered"]
    peak_point = max(points, key=lambda point: point["throughput"])
    knee_95 = find_first_throughput_95pct_point(points)
    knee_geo = find_knee_by_latency_geometry(points)

    summary["offered_min"] = f"{offered_min:.4f}"
    summary["offered_max"] = f"{offered_max:.4f}"
    summary["peak_throughput"] = f"{peak_point['throughput']:.6f}"
    summary["offered_at_peak_throughput"] = f"{peak_point['offered']:.4f}"

    if knee_95 is not None:
        summary["knee_ppp_95pct_throughput"] = int(knee_95["ppp"])
        summary["knee_offered_95pct_throughput"] = f"{knee_95['offered']:.4f}"
        summary["knee_latency_95pct_throughput"] = f"{knee_95['latency']:.6f}"

    if knee_geo is not None:
        summary["knee_ppp_latency_geometry"] = int(knee_geo["ppp"])
        summary["knee_offered_latency_geometry"] = f"{knee_geo['offered']:.4f}"
        summary["knee_latency_geometry"] = f"{knee_geo['latency']:.6f}"

    latency_min = points[0]["latency"]
    latency_max = points[-1]["latency"]
    throughput_min = points[0]["throughput"]
    throughput_max = points[-1]["throughput"]

    if latency_min > 0:
        summary["latency_growth_ratio_max_over_min"] = f"{(latency_max / latency_min):.6f}"
    summary["throughput_change_max_over_min"] = f"{(throughput_max - throughput_min):.6f}"

    summary["status"] = "ok" if len(points) >= 2 else "insufficient_points"
    return summary


def main() -> None:
    if not os.path.isdir(HOTSPOT_DIR):
        raise SystemExit(f"Hotspot directory not found: {HOTSPOT_DIR}")

    rows: List[Dict[str, object]] = []
    for entry in sorted(os.listdir(HOTSPOT_DIR)):
        topology_dir = os.path.join(HOTSPOT_DIR, entry)
        if not os.path.isdir(topology_dir) or not entry.startswith("slimfly_N"):
            continue
        curve_csv = os.path.join(topology_dir, "latency_vs_offered.csv")
        if not os.path.exists(curve_csv):
            rows.append(
                {
                    "topology": entry.replace("_hot20", ""),
                    "N": parse_topology_n(entry),
                    "num_points": 0,
                    "offered_min": "",
                    "offered_max": "",
                    "peak_throughput": "",
                    "offered_at_peak_throughput": "",
                    "knee_ppp_95pct_throughput": "",
                    "knee_offered_95pct_throughput": "",
                    "knee_latency_95pct_throughput": "",
                    "knee_ppp_latency_geometry": "",
                    "knee_offered_latency_geometry": "",
                    "knee_latency_geometry": "",
                    "latency_growth_ratio_max_over_min": "",
                    "throughput_change_max_over_min": "",
                    "status": "missing_curve_file",
                }
            )
            continue
        rows.append(summarize_curve(topology_dir, curve_csv))

    rows.sort(key=lambda row: int(row.get("N", 0)))

    columns = [
        "topology",
        "N",
        "num_points",
        "offered_min",
        "offered_max",
        "peak_throughput",
        "offered_at_peak_throughput",
        "knee_ppp_95pct_throughput",
        "knee_offered_95pct_throughput",
        "knee_latency_95pct_throughput",
        "knee_ppp_latency_geometry",
        "knee_offered_latency_geometry",
        "knee_latency_geometry",
        "latency_growth_ratio_max_over_min",
        "throughput_change_max_over_min",
        "status",
    ]

    with open(OUTPUT_CSV, "w", encoding="utf-8", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=columns)
        writer.writeheader()
        writer.writerows(rows)

    print(f"Wrote {len(rows)} rows to {OUTPUT_CSV}")


if __name__ == "__main__":
    main()
