import matplotlib.pyplot as plt
from parse_sa_log import parse_sa_log
import numpy as np
import matplotlib.ticker as mtick
from itertools import product
import scipy.io as io


def draw_sa(log_file: str, path):

    init_avg, init_var, init_balance, temperature, delta_cost, cost, taken, avg, var, balance = parse_sa_log(
        log_file)
    x = np.linspace(0, len(temperature), len(temperature))
    avg_relative = [item / init_avg for item in avg]
    var_relative = [item / init_var for item in var]
    balance_relative = [item / init_balance for item in balance]

    cost = np.array(cost)
    avg_relative = np.array(avg_relative)
    var_relative = np.array(var_relative)
    balance_relative = np.array(balance_relative)
    
    io.savemat(path, {'cost': cost, 'avg': avg_relative, 'var': var_relative, 'balance': balance_relative})
    


if __name__ == '__main__':

    topo_list = [
        (10, 'sa_swdc_ring_36_4'),
        (11, 'sa_swdc_ring_64_4'),
        (12, 'sa_swdc_ring_900_4'),
        (13, 'sa_swdc_ring_1600_4'),
        (14, 'sa_swdc_ring_2500_4'),
        (20, 'sa_jellyfish_36_4'),
        (21, 'sa_jellyfish_64_4'),
        (22, 'sa_jellyfish_900_4'),
        (23, 'sa_jellyfish_900_4_local90'),
        (24, 'sa_jellyfish_1600_4'),
        (25, 'sa_jellyfish_1600_4_local160'),
        (26, 'sa_jellyfish_2500_4'),
        (27, 'sa_jellyfish_2500_4_local250'),
        (38, 'sa_swdc_ring_36_6'),
        (39, 'sa_swdc_ring_64_6'),
        (40, 'sa_swdc_ring_900_6'),
        (41, 'sa_swdc_ring_1600_6'),
        (42, 'sa_swdc_ring_2500_6'),
        (48, 'sa_swdc_torus_36_6'),
        (49, 'sa_swdc_torus_64_6'),
        (50, 'sa_swdc_torus_900_6'),
        (51, 'sa_swdc_torus_1600_6'),
        (52, 'sa_swdc_torus_2500_6'),
        (58, 'sa_jellyfish_36_6'),
        (59, 'sa_jellyfish_64_6'),
        (60, 'sa_jellyfish_900_6'),
        (61, 'sa_jellyfish_900_6_local90'),
        (62, 'sa_jellyfish_1600_6'),
        (63, 'sa_jellyfish_1600_6_local160'),
        (64, 'sa_jellyfish_2500_6'),
        (65, 'sa_jellyfish_2500_6_local250'),
    ]

    for idx, file_name in topo_list:
        draw_sa(log_file='temp/221109_log/{:d}.log'.format(idx),
                path='temp/221109_figures_data/{:s}.mat'.format(file_name))
