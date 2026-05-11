import re
import os
import numpy as np


def parse_one(file_name: str):
    lut_mode = -1
    num_nodes = -1
    degree = -1
    root_select = -1
    cycle = -1
    apdl = -1.0
    num_vc = 1
    num_lut = 1
    channel_depth = 5
    with open(file_name, 'r') as f:
        line = f.readline()
        while line:
            if '**** Route Lut Mode is' in line:
                res = re.findall(r'\d+', line)
                assert len(res) == 1
                lut_mode = int(res[0])
            elif '----- start construct Graph' in line:
                res = re.findall(r'\d+', line)
                assert len(res) == 2
                num_nodes, degree = int(res[0]), int(res[1])
            elif '**** Root selection mode is' in line:
                res = re.findall(r'\d+', line)
                assert len(res) == 1
                root_select = int(res[0])
            elif 'Total cycle :' in line:
                res = re.findall(r'\d+', line)
                assert len(res) == 1
                cycle = int(res[0])
            elif 'Avg Packet Delivery Latency is :' in line:
                res = re.findall(r'-?\d+\.?\d*', line)
                assert len(res) == 1
                apdl = float(res[0])
            elif '**** Num of VC is' in line:
                res = re.findall(r'\d+', line)
                assert len(res) == 1
                num_vc = int(res[0])
            elif '**** Num of LUT is' in line:
                res = re.findall(r'\d+', line)
                assert len(res) == 1
                num_lut = int(res[0])
            elif '**** Depth of Channel is' in line:
                res = re.findall(r'\d+', line)
                assert len(res) == 1
                channel_depth = int(res[0])
            line = f.readline()

    return lut_mode, root_select, num_nodes, degree, cycle, apdl, num_vc, num_lut, channel_depth


def parse_deadlock_log(file_list: str):
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
        1800: 13,
        2000: 14,
        2200: 15,
        2400: 16,
        2600: 17,
        2800: 18,
        3000: 19
    }
    degree_dict = {4: 0, 6: 1, 8: 2}
    cycle_list = np.ones((2, 2, 20, 3), dtype=np.int32)
    apdl_list = np.ones((2, 2, 20, 3), dtype=np.float)
    for file in file_list:
        lut_mode, root_select, num_nodes, degree, cycle, apdl, num_vc, num_lut, channel_depth = parse_one(file_name=file)
        idx0, idx1, idx2, idx3 = lut_mode, root_select, node_dict[num_nodes], degree_dict[degree]
        cycle_list[idx0][idx1][idx2][idx3] = cycle
        apdl_list[idx0][idx1][idx2][idx3] = apdl
    return cycle_list, apdl_list

def parse_deadlock_2vc3_log(file_list: str):
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
    cycle_list = np.ones((3, 20, 3), dtype=np.int32)    # root_select, node_num, degree
    apdl_list = np.ones((3, 20, 3), dtype=np.float)
    for file in file_list:
        lut_mode, root_select, num_nodes, degree, cycle, apdl, num_vc, num_lut, channel_depth = parse_one(file_name=file)
        # allow parsing logs generated with VC=2 or VC=3
        assert lut_mode == 1 and (num_vc == 2 or num_vc == 3) and num_lut == 2 and channel_depth == 3
        idx0, idx1, idx2 = root_select, node_dict[num_nodes], degree_dict[degree]
        cycle_list[idx0][idx1][idx2] = cycle
        apdl_list[idx0][idx1][idx2] = apdl
    return cycle_list, apdl_list

if __name__ == '__main__':
    file_list = ['temp/deadlock_free/log/{:d}.log'.format(i) for i in range(96)]
    for i in range(1000, 1048):
        file_list.append('temp/deadlock_free/log/{:d}.log'.format(i))
    parse_deadlock_log(file_list=file_list)
