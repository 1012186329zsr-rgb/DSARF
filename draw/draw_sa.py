import matplotlib.pyplot as plt
from parse_sa_log import parse_sa_log
import numpy as np
import matplotlib.ticker as mtick
from itertools import product


def draw_sa(log_file: str, figure_file: str):

    init_avg, init_var, init_balance, temperature, delta_cost, cost, taken, avg, var, balance = parse_sa_log(
        log_file)
    x = np.linspace(0, len(temperature), len(temperature))
    avg_relative = [item / init_avg for item in avg]
    var_relative = [item / init_var for item in var]
    balance_relative = [item / init_balance for item in balance]

    font = {
        'family': 'serif',
        'weight': 'normal',
        'size': 10,
    }
    with plt.style.context(['science', 'ieee']):
        fig, ax = plt.subplots(1, 1)
        fig.set_size_inches(3.2, 2.4)
        # ax.plot(x, temperature, label='Temperature')
        # ax.plot(x, delta_cost, label='delta_cost')
        # ax[0].plot(x, cost, label='$cost$', linewidth=0.5)
        # ax[0].legend(prop={'size': 12}, loc=0)
        ax.plot(x, avg_relative, label='$SP_{avg}$', linewidth=0.5)
        ax.plot(x, var_relative, label='$SP_{var}$', linewidth=0.5)
        ax.plot(x, balance_relative, label='$PB$', linewidth=0.5)
        ax.legend(prop={'size': 8}, loc=0)

        # ax[1].set(xlabel='Iteration', fontdict=font)
        ax.set_xlabel('Iteration', fontdict=font)
        ax.set_ylabel(ylabel='Cost', fontdict=font)
        ax.set_ylim(0.3, 1.2)
        ax.set_xlim(0, len(temperature))
        # ax[1].set_ylabel(ylabel='Cost', fontdict=font)

        # ax.autoscale(tight=True)
        # ax[1].autoscale(tight=True)
        ax.yaxis.set_major_formatter(mtick.FormatStrFormatter('%.1f'))
        # ax[1].yaxis.set_major_formatter(mtick.FormatStrFormatter('%.2f'))
        ax.tick_params(labelsize=8)
        # ax[1].tick_params(labelsize=5)

        # fig.savefig('temp/figures/{:s}.pdf'.format(figure_file))
        fig.savefig(figure_file, dpi=5000)


if __name__ == '__main__':

    topo_list = [
        # (10, 'sa_swdc_ring_36_4'),
        # (11, 'sa_swdc_ring_64_4'),
        # (12, 'sa_swdc_ring_900_4'),
        # (13, 'sa_swdc_ring_1600_4'),
        # (14, 'sa_swdc_ring_2500_4'),
        # (20, 'sa_jellyfish_36_4'),
        # (21, 'sa_jellyfish_64_4'),
        (22, 'sa_jellyfish_900_4'),
        # (23, 'sa_jellyfish_900_4_local90'),
        (24, 'sa_jellyfish_1600_4'),
        # (25, 'sa_jellyfish_1600_4_local160'),
        (26, 'sa_jellyfish_2500_4'),
        # (27, 'sa_jellyfish_2500_4_local250'),
        # (38, 'sa_swdc_ring_36_6'),
        # (39, 'sa_swdc_ring_64_6'),
        # (40, 'sa_swdc_ring_900_6'),
        # (41, 'sa_swdc_ring_1600_6'),
        # (42, 'sa_swdc_ring_2500_6'),
        # (48, 'sa_swdc_torus_36_6'),
        # (49, 'sa_swdc_torus_64_6'),
        # (50, 'sa_swdc_torus_900_6'),
        # (51, 'sa_swdc_torus_1600_6'),
        # (52, 'sa_swdc_torus_2500_6'),
        # (58, 'sa_jellyfish_36_6'),
        # (59, 'sa_jellyfish_64_6'),
        (60, 'sa_jellyfish_900_6'),
        # (61, 'sa_jellyfish_900_6_local90'),
        (62, 'sa_jellyfish_1600_6'),
        # (63, 'sa_jellyfish_1600_6_local160'),
        (64, 'sa_jellyfish_2500_6'),
        # (65, 'sa_jellyfish_2500_6_local250'),
    ]

    for idx, file_name in topo_list:
        draw_sa(log_file='temp/221109_log/{:d}.log'.format(idx),
                figure_file='temp/221109_figures/{:s}.pdf'.format(file_name))
        # break