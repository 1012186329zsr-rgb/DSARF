import re
import os
import numpy as np


def parse_one(file_name: str):
    path_diversity_mode = 0
    load_balance_mode = 0
    cycle = -1
    apdl = -1.0
    num_nodes = 0
    degree = 0
    with open(file_name, 'r') as f:
        line = f.readline()
        while line:
            if '**** Path Diversity mode' in line:
                res = re.findall(r'-?\d+\.?\d*', line)
                assert len(res) == 1
                path_diversity_mode = int(res[0])
            elif '**** Load Balance mode is' in line:
                res = re.findall(r'\d+', line)
                assert len(res) == 1
                load_balance_mode = int(res[0])
                if path_diversity_mode == -1:
                    load_balance_mode = 0
            elif 'Total cycle :' in line:
                res = re.findall(r'\d+', line)
                assert len(res) == 1
                cycle = int(res[0])
            elif 'Avg Packet Delivery Latency is :' in line:
                if cycle == 0:
                    apdl = 0.0
                else:
                    res = re.findall(r'-?\d+\.?\d*', line)
                    assert len(res) == 1
                    apdl = float(res[0])
            elif '----- start construct Graph' in line:
                res = re.findall(r'\d+', line)
                assert len(res) == 2
                num_nodes, degree = int(res[0]), int(res[1])
            line = f.readline()

    return num_nodes, degree, path_diversity_mode, load_balance_mode, cycle, apdl


def parse_bit_reverse_log(file_list: str):
    # lut_mode; root_select; num_nodes; degree
    node_dict = {
        20: 0,
        40: 1,
        60: 2,
        80: 3,
        100: 4,
        200: 5,
        400: 6,
        600: 7,
        800: 8,
        1000: 9,
        1200: 10,
        1400: 11,
        1600: 12,
        1800: 13
    }
    degree_dict = {4: 0, 6: 1, 8: 2}
    # num_nodes, degree, ppp, load_balance_mode
    result = dict()
    for file in file_list:
        num_nodes, degree, path_diversity_mode, load_balance_mode, cycle, apdl = parse_one(
            file_name=file)
        idx0, idx1, idx2 = node_dict[num_nodes], degree_dict[degree], load_balance_mode
        if result.get(path_diversity_mode) is None:
            result[path_diversity_mode] = {
                'cycle': -np.ones((14, 3, 3), dtype=np.int32),
                'apdl': -np.ones((14, 3, 3), dtype=np.float32)
            }
        result[path_diversity_mode]['cycle'][idx0][idx1][idx2] = cycle
        result[path_diversity_mode]['apdl'][idx0][idx1][idx2] = apdl
    return result

