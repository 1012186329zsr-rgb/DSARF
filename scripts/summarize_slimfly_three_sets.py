#!/usr/bin/env python3
import argparse
import csv
import os
import re
from typing import Dict, List


REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BASE_RESULTS_DIR = os.path.join(REPO_ROOT, "results")
DEFAULT_DATASETS = ["slimfly", "slimfly_baseline", "slimfly_suite"]
DEFAULT_OUTPUT_NAME = "slimfly_three_sets_summary.csv"


def parse_summary(summary_path: str) -> Dict[str, object]:
    data: Dict[str, object] = {}
    if not os.path.exists(summary_path):
        return data

    with open(summary_path, "r", encoding="utf-8") as file:
        content = file.read()

    match = re.search(r"N=(\d+),\s*deg=(\d+)", content)
    if match:
        data["N"] = int(match.group(1))
        data["deg"] = int(match.group(2))

    match = re.search(
        r"max=(\d+),\s*min=(\d+),\s*mean=([\d\.]+),\s*median=([\d\.]+),\s*std=([\d\.]+),\s*q1=([\d\.]+),\s*q3=([\d\.]+)",
        content,
    )
    if match:
        data["Load_Max"] = int(match.group(1))
        data["Load_Min"] = int(match.group(2))
        data["Load_Mean"] = float(match.group(3))
        data["Load_Median"] = float(match.group(4))
        data["Load_Std"] = float(match.group(5))
        data["Load_Q1"] = float(match.group(6))
        data["Load_Q3"] = float(match.group(7))

    match = re.search(r"Gini=([\d\.]+)", content)
    if match:
        data["Gini"] = float(match.group(1))

    match = re.search(r"Total Packets\s*:\s*(\d+)", content)
    if match:
        data["Total_Packets"] = int(match.group(1))

    match = re.search(r"Total cycle\s*:\s*(\d+)", content)
    if match:
        data["Total_Cycle"] = int(match.group(1))

    match = re.search(r"Throughput\s*:\s*([\d\.]+)\s*packets/cycle", content)
    if match:
        data["Throughput"] = float(match.group(1))

    match = re.search(r"Avg Packet Delivery Latency is\s*:\s*([\d\.]+)", content)
    if match:
        data["Avg_Latency"] = float(match.group(1))

    match = re.search(r"Max Packet Delivery Latency is\s*:\s*(\d+)", content)
    if match:
        data["Max_Latency"] = int(match.group(1))

    return data


def parse_sim_log(sim_log_path: str) -> Dict[str, object]:
    data: Dict[str, object] = {}
    if not os.path.exists(sim_log_path):
        return data

    with open(sim_log_path, "r", encoding="utf-8") as file:
        content = file.read()

    match = re.search(r"seed\s*=\s*(\d+)", content)
    if match:
        data["Seed"] = int(match.group(1))

    match = re.search(r"ppp\s*=\s*(\d+/\d+)", content)
    if match:
        data["PPP"] = match.group(1)

    match = re.search(r"packet_num\s*=\s*(\d+)", content)
    if match:
        data["Packet_Num"] = int(match.group(1))

    match = re.search(r"Traffic Mode is\s*:\s*(-?\d+)", content)
    if match:
        data["Traffic_Mode"] = int(match.group(1))

    match = re.search(r"Path Diversity mode is\s*:\s*(-?\d+)", content)
    if match:
        data["Path_Diversity_Mode"] = int(match.group(1))

    match = re.search(r"Load Balance mode is\s*:\s*(-?\d+)", content)
    if match:
        data["Load_Balance_Mode"] = int(match.group(1))

    match = re.search(r"Num of VC is\s*:\s*(\d+)", content)
    if match:
        data["Num_VC"] = int(match.group(1))

    match = re.search(r"Num of LUT is\s*:\s*(\d+)", content)
    if match:
        data["Num_LUT"] = int(match.group(1))

    match = re.search(r"Depth of Channel is\s*:\s*(\d+)", content)
    if match:
        data["Channel_Depth"] = int(match.group(1))

    match = re.search(r"Total Packets\s*:\s*(\d+)", content)
    if match:
        data["Total_Packets"] = int(match.group(1))

    match = re.search(r"Total cycle\s*:\s*(\d+)", content)
    if match:
        data["Total_Cycle"] = int(match.group(1))

    match = re.search(r"Throughput\s*:\s*([\d\.]+)\s*packets/cycle", content)
    if match:
        data["Throughput"] = float(match.group(1))

    match = re.search(r"Avg Packet Delivery Latency is\s*:\s*([\d\.]+)", content)
    if match:
        data["Avg_Latency"] = float(match.group(1))

    match = re.search(r"Max Packet Delivery Latency is\s*:\s*(\d+)", content)
    if match:
        data["Max_Latency"] = int(match.group(1))

    match = re.search(r"Using time\s*=\s*([\d\.]+)\s*s", content)
    if match:
        data["Sim_Time_s"] = float(match.group(1))

    return data


def parse_case_name(case_name: str) -> Dict[str, object]:
    data: Dict[str, object] = {"Case_Name": case_name}
    match = re.match(r"slimfly_N(\d+)_D(\d+)_(.+)", case_name)
    if match:
        data["N"] = int(match.group(1))
        data["deg"] = int(match.group(2))
        data["Routing_Tag"] = match.group(3)
    return data


def collect_dataset_rows(dataset_name: str) -> List[Dict[str, object]]:
    dataset_dir = os.path.join(BASE_RESULTS_DIR, dataset_name)
    if not os.path.isdir(dataset_dir):
        return []

    rows: List[Dict[str, object]] = []
    for entry in sorted(os.listdir(dataset_dir)):
        case_dir = os.path.join(dataset_dir, entry)
        if not os.path.isdir(case_dir) or not entry.startswith("slimfly_N"):
            continue

        row: Dict[str, object] = {"Dataset": dataset_name}
        row.update(parse_case_name(entry))

        summary_path = os.path.join(case_dir, "summary.txt")
        sim_log_path = os.path.join(case_dir, "sim_log.txt")

        row.update(parse_summary(summary_path))
        row.update(parse_sim_log(sim_log_path))
        rows.append(row)

    return rows


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Summarize multiple slimfly result folders into one CSV."
    )
    parser.add_argument(
        "--datasets",
        nargs="+",
        default=DEFAULT_DATASETS,
        help="Dataset folder names under results/ (default: slimfly slimfly_baseline slimfly_suite)",
    )
    parser.add_argument(
        "--output",
        default=DEFAULT_OUTPUT_NAME,
        help="Output CSV filename under results/",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    output_csv = os.path.join(BASE_RESULTS_DIR, args.output)

    all_rows: List[Dict[str, object]] = []
    for dataset_name in args.datasets:
        all_rows.extend(collect_dataset_rows(dataset_name))

    all_rows.sort(key=lambda row: (row.get("Dataset", ""), int(row.get("N", 0))))

    columns = [
        "Dataset",
        "Case_Name",
        "Routing_Tag",
        "N",
        "deg",
        "Seed",
        "PPP",
        "Packet_Num",
        "Traffic_Mode",
        "Path_Diversity_Mode",
        "Load_Balance_Mode",
        "Num_VC",
        "Num_LUT",
        "Channel_Depth",
        "Total_Packets",
        "Total_Cycle",
        "Throughput",
        "Avg_Latency",
        "Max_Latency",
        "Sim_Time_s",
        "Load_Mean",
        "Load_Max",
        "Load_Min",
        "Load_Median",
        "Load_Q1",
        "Load_Q3",
        "Load_Std",
        "Gini",
    ]

    os.makedirs(BASE_RESULTS_DIR, exist_ok=True)
    with open(output_csv, "w", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(file, fieldnames=columns)
        writer.writeheader()
        for row in all_rows:
            writer.writerow({column: row.get(column, "") for column in columns})

    print(f"Wrote {len(all_rows)} rows to {output_csv}")


if __name__ == "__main__":
    main()
