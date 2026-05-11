#!/usr/bin/env python3
import os
import re
import csv
import sys

RESULTS_DIR = "results/slimfly_suite"
OUTPUT_CSV = os.path.join(RESULTS_DIR, "summary_table.csv")

def parse_summary(filepath):
    data = {}
    if not os.path.exists(filepath):
        return data
    
    with open(filepath, 'r') as f:
        content = f.read()
        
        # N=98, deg=11
        m = re.search(r'N=(\d+), deg=(\d+)', content)
        if m:
            data['N'] = int(m.group(1))
            data['deg'] = int(m.group(2))
            
        # max=76731, min=13989, mean=37081.2245, median=33821.0000, std=14255.2934, q1=25000.0000, q3=45000.0000
        m = re.search(r'max=(\d+), min=(\d+), mean=([\d\.]+), median=([\d\.]+), std=([\d\.]+)(?:, q1=([\d\.]+), q3=([\d\.]+))?', content)
        if m:
            data['Load_Max'] = int(m.group(1))
            data['Load_Min'] = int(m.group(2))
            data['Load_Mean'] = float(m.group(3))
            data['Load_Median'] = float(m.group(4))
            data['Load_Std'] = float(m.group(5))
            if m.group(6) and m.group(7):
                data['Load_Q1'] = float(m.group(6))
                data['Load_Q3'] = float(m.group(7))
            
        # Gini=0.214164
        m = re.search(r'Gini=([\d\.]+)', content)
        if m:
            data['Gini'] = float(m.group(1))
            
    return data

def parse_sim_log(filepath):
    data = {}
    if not os.path.exists(filepath):
        return data
        
    with open(filepath, 'r') as f:
        content = f.read()
        
        # Avg Packet Delivery Latency is : 83.701183
        m = re.search(r'Avg Packet Delivery Latency is : ([\d\.]+)', content)
        if m:
            data['Avg_Latency'] = float(m.group(1))
            
        # Max Packet Delivery Latency is : 33763
        m = re.search(r'Max Packet Delivery Latency is : (\d+)', content)
        if m:
            data['Max_Latency'] = int(m.group(1))
            
        # Using time = 53.890207 s
        m = re.search(r'Using time = ([\d\.]+) s', content)
        if m:
            data['Sim_Time_s'] = float(m.group(1))
            
    return data

def main():
    results = []
    
    if not os.path.exists(RESULTS_DIR):
        print(f"Directory {RESULTS_DIR} not found.")
        return

    for entry in os.listdir(RESULTS_DIR):
        dir_path = os.path.join(RESULTS_DIR, entry)
        if os.path.isdir(dir_path) and entry.startswith("slimfly_N"):
            # Expected structure: results/slimfly_suite/slimfly_N98_D11_treeturn_nonmin/
            summary_path = os.path.join(dir_path, "summary.txt")
            sim_log_path = os.path.join(dir_path, "sim_log.txt")
            
            # Fallback for N98 if sim_log is outside (based on previous observation, though user might have re-run)
            if not os.path.exists(sim_log_path):
                # Try looking for it in the parent dir with suffix
                parent_log = os.path.join(RESULTS_DIR, entry + "_sim_log.txt")
                if os.path.exists(parent_log):
                    sim_log_path = parent_log

            entry_data = {}
            entry_data.update(parse_summary(summary_path))
            entry_data.update(parse_sim_log(sim_log_path))
            
            if 'N' in entry_data:
                results.append(entry_data)

    # Sort by N
    results.sort(key=lambda x: x.get('N', 0))
    
    # Define columns
    columns = ['N', 'deg', 'Avg_Latency', 'Max_Latency', 'Load_Mean', 'Load_Max', 'Load_Min', 'Load_Median', 'Load_Q1', 'Load_Q3', 'Load_Std', 'Gini', 'Sim_Time_s']
    
    # Write CSV
    with open(OUTPUT_CSV, 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=columns)
        writer.writeheader()
        for row in results:
            # Filter row to only include defined columns
            filtered_row = {k: row.get(k, '') for k in columns}
            writer.writerow(filtered_row)
            
    print(f"Summary table saved to {OUTPUT_CSV}")
    
    # Print Markdown Table
    print("\n### Simulation Results Summary")
    print("| N | Degree | Avg Latency | Max Latency | Mean Load | Max Load | Min Load | Median Load | Q1 Load | Q3 Load | Std Load | Gini | Time (s) |")
    print("|---|---|---|---|---|---|---|---|---|---|---|---|---|")
    for row in results:
        avg_lat = f"{row.get('Avg_Latency', 0):.2f}" if 'Avg_Latency' in row else "N/A"
        max_lat = row.get('Max_Latency', "N/A")
        mean_load = f"{row.get('Load_Mean', 0):.1f}" if 'Load_Mean' in row else "N/A"
        max_load = row.get('Load_Max', "N/A")
        min_load = row.get('Load_Min', "N/A")
        median_load = f"{row.get('Load_Median', 0):.1f}" if 'Load_Median' in row else "N/A"
        q1_load = f"{row.get('Load_Q1', 0):.1f}" if 'Load_Q1' in row else "N/A"
        q3_load = f"{row.get('Load_Q3', 0):.1f}" if 'Load_Q3' in row else "N/A"
        std_load = f"{row.get('Load_Std', 0):.1f}" if 'Load_Std' in row else "N/A"
        gini = f"{row.get('Gini', 0):.4f}" if 'Gini' in row else "N/A"
        sim_time = f"{row.get('Sim_Time_s', 0):.1f}" if 'Sim_Time_s' in row else "N/A"
        
        print(f"| {row.get('N')} | {row.get('deg')} | {avg_lat} | {max_lat} | {mean_load} | {max_load} | {min_load} | {median_load} | {q1_load} | {q3_load} | {std_load} | {gini} | {sim_time} |")

if __name__ == "__main__":
    main()
