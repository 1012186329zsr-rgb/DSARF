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
        # ax.set_ylim(0.3, 1.2)
        # ax.set_xlim(0, len(temperature))
        # ax[1].set_ylabel(ylabel='Cost', fontdict=font)

        ax.autoscale(tight=True)
        # ax[1].autoscale(tight=True)
        ax.yaxis.set_major_formatter(mtick.FormatStrFormatter('%.1f'))
        # ax[1].yaxis.set_major_formatter(mtick.FormatStrFormatter('%.2f'))
        ax.tick_params(labelsize=8)
        # ax[1].tick_params(labelsize=5)

        # fig.savefig('temp/figures/{:s}.pdf'.format(figure_file))
        fig.savefig(figure_file, dpi=5000)


if __name__ == '__main__':

    sa_list = [
        (' 1 900 1 2 ', 'sa_swdc_ring_node900_degree4'),
        (' 1 900 1 4 ', 'sa_swdc_ring_node900_degree6'),
        (' 1 1600 1 2 ', 'sa_swdc_ring_node1600_degree4'),
        (' 1 1600 1 4 ', 'sa_swdc_ring_node1600_degree6'),
        (' 1 2500 1 2 ', 'sa_swdc_ring_node2500_degree4'),
        (' 1 2500 1 4 ', 'sa_swdc_ring_node2500_degree6'),
        (' 1 30 2 2 ', 'sa_swdc_torus_node900_degree6'),
        (' 1 40 2 2 ', 'sa_swdc_torus_node1600_degree6'),
        (' 1 50 2 2 ', 'sa_swdc_torus_node2500_degree6'),
        (' 0 900 4 ', 'sa_jellyfish_node900_degree4'),
        (' 0 900 6 ', 'sa_jellyfish_node900_degree6'),
        (' 0 1600 4 ', 'sa_jellyfish_node1600_degree4'),
        (' 0 1600 6 ', 'sa_jellyfish_node1600_degree6'),
        (' 0 2500 4 ', 'sa_jellyfish_node2500_degree4'),
        (' 0 2500 6 ', 'sa_jellyfish_node2500_degree6'),
    ]

    for _, file_name in sa_list:
        draw_sa(log_file='temp/sa_gen/{:s}.txt'.format(file_name),
                figure_file='temp/sa_gen/pdf_{:s}.pdf'.format(file_name))