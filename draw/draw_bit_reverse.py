import matplotlib.pyplot as plt
from parse_bit_reverse_log import parse_bit_reverse_log
import numpy as np
import matplotlib.ticker as mtick
from itertools import product


def draw_adaptive_2vc3(log_file_list: list, figure_file: str, idx, degree: int, mode: str):

    result = parse_bit_reverse_log(log_file_list)
    # index |   diversity       |       load balance
    #   0   |   -1              |           /
    #   1   |   0(minimal)      |           0(equal)
    #   2   |   0(minimal)      |           1(local)
    #   3   |   0(minimal)      |           2(non-local)
    #   4   |   (non-minimal)   |           1(local)
    #   5   |   (non-minimal)   |           2(non-local)

    x = np.linspace(0, idx, idx)

    x_ticks = [20, 40, 60, 80, 100, 200, 400,
               600, 800, 1000, 1200, 1400, 1600, 1800]
    x_ticks = [str(i) for i in x_ticks]
    
    degree_dict = {4:0, 6:1, 8:2}

    font = {
        'family': 'serif',
        'weight': 'normal',
        'size': 8,
    }
    with plt.style.context(['science', 'ieee']):
        plt.rcParams['font.size'] = 6
        fig, ax = plt.subplots()
        ########################################################
        # Degree = 4
        deee = degree_dict[degree]
        ax.plot(x, result[-1][mode][0:idx, deee, 0], label='Deterministic Routing', marker='s', ms=3, color='k', linestyle='solid')
        ax.plot(x, result[0][mode][0:idx, deee, 0], label='Non-deterministic minimal routing', marker='o', ms=3, color='k', linestyle='dashed')
        ax.plot(x, result[0][mode][0:idx, deee, 1], label='Local congestion aware minimal routing', marker='^', ms=3, color='k', linestyle='dashed')
        ax.plot(x, result[1][mode][0:idx, deee, 2], label='Non-local congestion aware non-minimal routing', marker='p', ms=3, color='k', linestyle='dotted')

        # plt.yscale('log')
        ax.legend(prop={'size': 6}, loc=0)
        ax.set_xlabel('Network Size', fontdict=font)
        if mode == 'cycle':
            ax.set_ylabel(ylabel='Latency/cycles', fontdict=font)
        elif mode == 'apdl':
            ax.set_ylabel(ylabel='Average Packet Delivery Latency/cycles', fontdict=font)
        else:
            raise ValueError
        ax.set_xticks(x)
        ax.set_xticklabels(x_ticks[:idx], rotation=30, fontsize='small')
        ax.tick_params(labelsize=6)
        fig.savefig(figure_file, dpi=5000)


if __name__ == '__main__':

    # idx = 13

    # file_list = [
    #     'temp/bit_reverse/log/{:d}.log'.format(i) for i in range(idx * 3 * 4)]
    # draw_adaptive_2vc3(file_list, 'temp/bit_reverse/figures/degree4_apdl.jpg', idx, degree=4, mode='apdl')
    
    idx = 13

    file_list = [
        'temp/bit_reverse/log/{:d}.log'.format(i) for i in range(idx * 4 * 3)]
    draw_adaptive_2vc3(file_list, 'temp/bit_reverse/figures/bit_reverse_degree4_apdl.pdf', idx, degree=4, mode='apdl')
