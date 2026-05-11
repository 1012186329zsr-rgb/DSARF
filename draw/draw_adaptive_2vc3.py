import matplotlib.pyplot as plt
from parse_adaptive_log import parse_adaptive_log
import numpy as np
import matplotlib.ticker as mtick
from itertools import product
from matplotlib.lines import Line2D


def draw_adaptive_2vc3(log_file_list: list, figure_file: str, idx):

    result = parse_adaptive_log(log_file_list)
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
    
    d_dict = {
        0: 4,
        1: 6,
        2: 8
    }

    font = {
        'family': 'serif',
        'weight': 'normal',
        'size': 10,
    }
    # with plt.style.context(['science', 'ieee']):
    #     plt.rcParams['font.size'] = 6
    #     fig, ax = plt.subplots()
    #     deee = 0
    #     fig.set_size_inches(3.2 * 2.1, 2.4 * 1.5)
    #     ax.plot(x, result[-1]['cycle'][0:idx, deee, 0, 0], label='Deterministic', marker='s', ms=3, color='k', linestyle='solid')
    #     ax.plot(x, result[0]['cycle'][0:idx, deee, 0, 0], label='Non-deterministic minimal routing', marker='o', ms=3, color='k', linestyle='dashed')
    #     ax.plot(x, result[0]['cycle'][0:idx, deee, 0, 1], label='Local congestion aware minimal routing', marker='^', ms=3, color='k', linestyle='dashed')
    #     # ax.plot(x, result[0]['cycle'][0:idx, deee, 0, 2], label='Minimal-diversity \& Non-local congestion aware', marker='h', ms=3, color='k', linestyle='dashed')
    #     # ax.plot(x, result[1]['cycle'][0:idx, deee, 0, 1], label='Non-Local congestion aware minimal routing', marker='d', ms=3, color='k', linestyle='dotted')
    #     ax.plot(x, result[1]['cycle'][0:idx, deee, 0, 2], label='Non-local congestion aware non-minimal routing', marker='p', ms=3, color='k', linestyle='dotted')

    #     ax.plot(x, result[-1]['cycle'][0:idx, deee, 1, 0], label='Deterministic Routing', marker='s', ms=3, color='r', linestyle='solid')
    #     ax.plot(x, result[0]['cycle'][0:idx, deee, 1, 0], label='Non-deterministic minimal routing', marker='o', ms=3, color='r', linestyle='dashed')
    #     ax.plot(x, result[0]['cycle'][0:idx, deee, 1, 1], label='Local congestion aware minimal routing', marker='^', ms=3, color='r', linestyle='dashed')
    #     # ax.plot(x, result[0]['cycle'][0:idx, deee, 1, 2], label='Minimal-diversity \& Non-local congestion aware', marker='h', ms=3, color='r', linestyle='dashed')
    #     # ax.plot(x, result[1]['cycle'][0:idx, deee, 1, 1], label='Non-Local congestion aware minimal routing', marker='d', ms=3, color='r', linestyle='dotted')
    #     ax.plot(x, result[1]['cycle'][0:idx, deee, 1, 2], label='Non-local congestion aware non-minimal routing', marker='p', ms=3, color='r', linestyle='dotted')

    #     ax.plot(x, result[-1]['cycle'][0:idx, deee, 2, 0], label='Deterministic Routing', marker='s', ms=3, color='b', linestyle='solid')
    #     ax.plot(x, result[0]['cycle'][0:idx, deee, 2, 0], label='Non-deterministic minimal routing', marker='o', ms=3, color='b', linestyle='dashed')
    #     ax.plot(x, result[0]['cycle'][0:idx, deee, 2, 1], label='Local congestion aware minimal routing', marker='^', ms=3, color='b', linestyle='dashed')
    #     # ax.plot(x, result[0]['cycle'][0:idx, deee, 2, 2], label='Minimal-diversity \& Non-local congestion aware', marker='h', ms=3, color='b', linestyle='dashed')
    #     # ax.plot(x, result[1]['cycle'][0:idx, deee, 2, 1], label='Non-Local congestion aware minimal routing', marker='d', ms=3, color='b', linestyle='dotted')
    #     ax.plot(x, result[1]['cycle'][0:idx, deee, 2, 2], label='Non-local congestion aware non-minimal routing', marker='p', ms=3, color='b', linestyle='dotted')

    #     ax.legend(prop={'size': 8}, loc=9, ncol=3, bbox_to_anchor=(1.01, 1.3))
    #     ax.set_xlabel('Network Size', fontdict=font)
    #     ax.set_ylabel(ylabel='Latency/cycles', fontdict=font)
    #     ax.set_xticks(x)
    #     ax.set_xticklabels(x_ticks[:idx], rotation=40, fontsize='small')
    #     ax.tick_params(labelsize=5)
    #     fig.savefig(figure_file + 'legend.pdf', dpi=5000)
    for deee in [0, 1, 2]:
        with plt.style.context(['science', 'ieee']):
            plt.rcParams['font.size'] = 6
            fig, ax = plt.subplots()
            fig.set_size_inches(3.2, 2.4)
            ax.plot(x, result[-1]['cycle'][0:idx, deee, 0, 0], label='Deterministic', marker='s', ms=3, color='k', linestyle='dotted')
            ax.plot(x, result[0]['cycle'][0:idx, deee, 0, 0], label='Non-deterministic \& minimal', marker='o', ms=3, color='k', linestyle='dotted')
            ax.plot(x, result[0]['cycle'][0:idx, deee, 0, 1], label='Local congestion \& minimal', marker='^', ms=3, color='k', linestyle='dotted')
            # ax.plot(x, result[0]['cycle'][0:idx, deee, 0, 2], label='Minimal-diversity \& Non-local congestion aware', marker='h', ms=3, color='k', linestyle='dashed')
            # ax.plot(x, result[1]['cycle'][0:idx, deee, 0, 1], label='Non-Local congestion aware minimal routing', marker='d', ms=3, color='k', linestyle='dotted')
            ax.plot(x, result[1]['cycle'][0:idx, deee, 0, 2], label='Non-local congestion \& non-minimal', marker='p', ms=3, color='k', linestyle='dotted')

            ax.plot(x, result[-1]['cycle'][0:idx, deee, 1, 0], label='Deterministic', marker='s', ms=3, color='r', linestyle='dashed')
            ax.plot(x, result[0]['cycle'][0:idx, deee, 1, 0], label='Non-deterministic \& minimal', marker='o', ms=3, color='r', linestyle='dashed')
            ax.plot(x, result[0]['cycle'][0:idx, deee, 1, 1], label='Local congestion \& minimal', marker='^', ms=3, color='r', linestyle='dashed')
            # ax.plot(x, result[0]['cycle'][0:idx, deee, 1, 2], label='Minimal-diversity \& Non-local congestion aware', marker='h', ms=3, color='r', linestyle='dashed')
            # ax.plot(x, result[1]['cycle'][0:idx, deee, 1, 1], label='Non-Local congestion aware minimal routing', marker='d', ms=3, color='r', linestyle='dotted')
            ax.plot(x, result[1]['cycle'][0:idx, deee, 1, 2], label='Non-local congestion \& non-minimal', marker='p', ms=3, color='r', linestyle='dashed')

            ax.plot(x, result[-1]['cycle'][0:idx, deee, 2, 0], label='Deterministic', marker='s', ms=3, color='b', linestyle='solid')
            ax.plot(x, result[0]['cycle'][0:idx, deee, 2, 0], label='Non-deterministic \& minimal', marker='o', ms=3, color='b', linestyle='solid')
            ax.plot(x, result[0]['cycle'][0:idx, deee, 2, 1], label='Local congestion \& minimal', marker='^', ms=3, color='b', linestyle='solid')
            # ax.plot(x, result[0]['cycle'][0:idx, deee, 2, 2], label='Minimal-diversity \& Non-local congestion aware', marker='h', ms=3, color='b', linestyle='dashed')
            # ax.plot(x, result[1]['cycle'][0:idx, deee, 2, 1], label='Non-Local congestion aware minimal routing', marker='d', ms=3, color='b', linestyle='dotted')
            ax.plot(x, result[1]['cycle'][0:idx, deee, 2, 2], label='Non-local congestion \& non-minimal', marker='p', ms=3, color='b', linestyle='solid')

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
            # if deee == 0:
            #     ax.legend(prop={'size': 5}, loc=9, ncol=3, bbox_to_anchor=(1.2, 1.3))
            # else:
            ax.legend(handles=lines, prop={'size': 6}, loc=0)
            # ax.legend(prop={'size': 6}, loc=0)
            ax.set_xlabel('Network Size', fontdict=font)
            ax.set_ylabel(ylabel='Latency/cycles', fontdict=font)
            ax.set_xticks(x)
            ax.set_xticklabels(x_ticks[:idx], rotation=40, fontsize='small')
            ax.tick_params(labelsize=5)
            fig.savefig(figure_file + 'adaptive_uniform_random_degree{:d}_cycle.pdf'.format(d_dict[deee]), dpi=5000)
            
            #################################
            plt.rcParams['font.size'] = 6
            fig, ax = plt.subplots()
            ax.plot(x, result[-1]['apdl'][0:idx, deee, 0, 0], label='Deterministic Routing', marker='s', ms=3, color='k', linestyle='dotted')
            ax.plot(x, result[0]['apdl'][0:idx, deee, 0, 0], label='Non-deterministic minimal routing', marker='o', ms=3, color='k', linestyle='dotted')
            ax.plot(x, result[0]['apdl'][0:idx, deee, 0, 1], label='Local congestion aware minimal routing', marker='^', ms=3, color='k', linestyle='dotted')
            # ax.plot(x, result[0]['apdl'][0:idx, deee, 0, 2], label='Minimal-diversity \& Non-local congestion aware', marker='h', ms=3, color='k', linestyle='dashed')
            # ax.plot(x, result[1]['apdl'][0:idx, deee, 0, 1], label='Non-Local congestion aware minimal routing', marker='d', ms=3, color='k', linestyle='dotted')
            ax.plot(x, result[1]['apdl'][0:idx, deee, 0, 2], label='Non-local congestion aware non-minimal routing', marker='p', ms=3, color='k', linestyle='dotted')

            ax.plot(x, result[-1]['apdl'][0:idx, deee, 1, 0], label='Deterministic Routing', marker='s', ms=3, color='r', linestyle='dashed')
            ax.plot(x, result[0]['apdl'][0:idx, deee, 1, 0], label='Non-deterministic minimal routing', marker='o', ms=3, color='r', linestyle='dashed')
            ax.plot(x, result[0]['apdl'][0:idx, deee, 1, 1], label='Local congestion aware minimal routing', marker='^', ms=3, color='r', linestyle='dashed')
            # ax.plot(x, result[0]['apdl'][0:idx, deee, 1, 2], label='Minimal-diversity \& Non-local congestion aware', marker='h', ms=3, color='r', linestyle='dashed')
            # ax.plot(x, result[1]['apdl'][0:idx, deee, 1, 1], label='Non-Local congestion aware minimal routing', marker='d', ms=3, color='r', linestyle='dotted')
            ax.plot(x, result[1]['apdl'][0:idx, deee, 1, 2], label='Non-local congestion aware non-minimal routing', marker='p', ms=3, color='r', linestyle='dashed')

            ax.plot(x, result[-1]['apdl'][0:idx, deee, 2, 0], label='Deterministic Routing', marker='s', ms=3, color='b', linestyle='solid')
            ax.plot(x, result[0]['apdl'][0:idx, deee, 2, 0], label='Non-deterministic minimal routing', marker='o', ms=3, color='b', linestyle='solid')
            ax.plot(x, result[0]['apdl'][0:idx, deee, 2, 1], label='Local congestion aware minimal routing', marker='^', ms=3, color='b', linestyle='solid')
            # ax.plot(x, result[0]['apdl'][0:idx, deee, 2, 2], label='Minimal-diversity \& Non-local congestion aware', marker='h', ms=3, color='b', linestyle='dashed')
            # ax.plot(x, result[1]['apdl'][0:idx, deee, 2, 1], label='Non-Local congestion aware minimal routing', marker='d', ms=3, color='b', linestyle='dotted')
            ax.plot(x, result[1]['apdl'][0:idx, deee, 2, 2], label='Non-local congestion aware non-minimal routing', marker='p', ms=3, color='b', linestyle='solid')

            # plt.yscale('log')
            # ax.legend(prop={'size': 5}, loc=0)
            ax.set_xlabel('Network Size', fontdict=font)
            ax.set_ylabel(ylabel='ADPL/cycles', fontdict=font)
            ax.set_xticks(x)
            ax.set_xticklabels(x_ticks[:idx], rotation=40, fontsize='small')
            ax.tick_params(labelsize=5)
            fig.savefig(figure_file + 'adaptive_uniform_random_degree{:d}_apdl.pdf'.format(d_dict[deee]), dpi=5000)

if __name__ == '__main__':

    idx = 13

    file_list = [
        'temp/adaptive/2vc3/{:d}.log'.format(i) for i in range(idx * 3 * 3 * 4)]
    for i in range(idx * 3 * 3 * 2):
        file_list.append('temp/adaptive/2vc3_non_local/{:d}.log'.format(i))
    draw_adaptive_2vc3(file_list, 'temp/adaptive/figures/', idx)
