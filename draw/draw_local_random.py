import matplotlib.pyplot as plt
from parse_local_random_log import parse_local_random_log
import numpy as np
import matplotlib.ticker as mtick
from itertools import product
from matplotlib.lines import Line2D


def draw_adaptive_2vc3(log_file_list: list, figure_file: str, idx):

    result = parse_local_random_log(log_file_list)
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
    for degree, mode in product([4, 6, 8], ['cycle', 'apdl']):
        with plt.style.context(['science', 'ieee']):
            plt.rcParams['font.size'] = 6
            fig, ax = plt.subplots()
            fig.set_size_inches(3.2, 2.4)
            ########################################################
            deee = degree_dict[degree]
            ax.plot(x, result[-1][mode][0:idx, deee, 0, 0], label='Deterministic', marker='s', ms=3, color='k', linestyle='dotted')
            ax.plot(x, result[0][mode][0:idx, deee, 0, 0], label='Non-deterministic \& minimal', marker='o', ms=3, color='k', linestyle='dotted')
            ax.plot(x, result[0][mode][0:idx, deee, 0, 1], label='Local congestion aware \& minimal', marker='^', ms=3, color='k', linestyle='dotted')
            # ax.plot(x, result[0][mode][0:idx, deee, 0, 2], label='Minimal-diversity \& Non-local congestion aware', marker='h', ms=3, color='k', linestyle='dashed')
            # ax.plot(x, result[1][mode][0:idx, deee, 0, 1], label='Non-Local congestion aware minimal', marker='d', ms=3, color='k', linestyle='dotted')
            ax.plot(x, result[1][mode][0:idx, deee, 0, 2], label='Non-local congestion aware \& non-minimal', marker='p', ms=3, color='k', linestyle='dotted')

            ax.plot(x, result[-1][mode][0:idx, deee, 1, 0], label='Deterministic', marker='s', ms=3, color='r', linestyle='dashed')
            ax.plot(x, result[0][mode][0:idx, deee, 1, 0], label='Non-deterministic \& minimal', marker='o', ms=3, color='r', linestyle='dashed')
            ax.plot(x, result[0][mode][0:idx, deee, 1, 1], label='Local congestion aware \& minimal', marker='^', ms=3, color='r', linestyle='dashed')
            # ax.plot(x, result[0][mode][0:idx, deee, 1, 2], label='Minimal-diversity \& Non-local congestion aware', marker='h', ms=3, color='r', linestyle='dashed')
            # ax.plot(x, result[1][mode][0:idx, deee, 1, 1], label='Non-Local congestion aware minimal', marker='d', ms=3, color='r', linestyle='dotted')
            ax.plot(x, result[1][mode][0:idx, deee, 1, 2], label='Non-local congestion aware \& non-minimal', marker='p', ms=3, color='r', linestyle='dashed')

            ax.plot(x, result[-1][mode][0:idx, deee, 2, 0], label='Deterministic', marker='s', ms=3, color='b', linestyle='solid')
            ax.plot(x, result[0][mode][0:idx, deee, 2, 0], label='Non-deterministic \& minimal', marker='o', ms=3, color='b', linestyle='solid')
            ax.plot(x, result[0][mode][0:idx, deee, 2, 1], label='Local congestion aware \& minimal', marker='^', ms=3, color='b', linestyle='solid')
            # ax.plot(x, result[0][mode][0:idx, deee, 2, 2], label='Minimal-diversity \& Non-local congestion aware', marker='h', ms=3, color='b', linestyle='dashed')
            # ax.plot(x, result[1][mode][0:idx, deee, 2, 1], label='Non-Local congestion aware minimal', marker='d', ms=3, color='b', linestyle='dotted')
            ax.plot(x, result[1][mode][0:idx, deee, 2, 2], label='Non-local congestion aware \& non-minimal', marker='p', ms=3, color='b', linestyle='solid')

            lines = [
                Line2D([0], [0], color='w', markerfacecolor='b', marker='s', ms=5, linestyle='solid', label='Deterministic'),
                Line2D([0], [0], color='w', markerfacecolor='b', marker='o', ms=5, linestyle='solid', label='Non-deterministic \& minimal'),
                Line2D([0], [0], color='w', markerfacecolor='b', marker='^', ms=5, linestyle='solid', label='Local congestion aware \& minimal'),
                Line2D([0], [0], color='w', markerfacecolor='b', marker='p', ms=5, linestyle='solid', label='Non-local congestion aware \& non-minimal'),
                Line2D([0], [0], color='b', ms=3, linestyle='solid', label='$p=10\%$'),
                Line2D([0], [0], color='r', ms=3, linestyle='dashed', label='$p=5\%$'),
                Line2D([0], [0], color='k', ms=3, linestyle='dotted', label='$p=1\%$'),
            ]
            
            # plt.yscale('log')
            if mode == 'cycle':
                # ax.legend(prop={'size': 6}, loc=0)
                ax.legend(handles=lines, prop={'size': 6}, loc=0)
            # ax.legend(prop={'size': 5}, loc=0, ncol=3, bbox_to_anchor=(1, 1))
            ax.set_xlabel('Network Size', fontdict=font)
            if mode == 'cycle':
                ax.set_ylabel(ylabel='Latency/cycles', fontdict=font)
            elif mode == 'apdl':
                ax.set_ylabel(ylabel='Average Packet Delivery Latency/cycles', fontdict=font)
            else:
                raise ValueError
            ax.set_xticks(x)
            ax.set_xticklabels(x_ticks[:idx], rotation=40, fontsize='small')
            ax.tick_params(labelsize=5)
            # ax.set_yscale('log')
            fig.savefig(figure_file + 'degree{:d}_{:s}.pdf'.format(degree, mode), dpi=5000)


if __name__ == '__main__':

    idx = 13

    file_list = [
        'temp/local_random_lam64/log/{:d}.log'.format(i) for i in range(idx * 3 * 3 * 4)]
    draw_adaptive_2vc3(file_list, 'temp/local_random_lam64/figures/lam64_', idx)
