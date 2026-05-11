from functools import total_ordering
from platform import node
from re import T
from traceback import print_tb
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.ticker as mtick
from itertools import product
from copy import deepcopy
from data221110 import data221110 as data


def draw_bar0(file_name: str):
    labels = ['2D-Torus', 'SWDC-Ring', 'SA-SWDC-Ring', 'SA-SWDC-Ring-Local',
              'Jellyfish', 'SA-Jellyfish', 'SA-Jellyfish-Local',  # degree = 4
              'Spinnaker', 'SWDC-Ring', 'SA-SWDC-Ring', 'SA-SWDC-Ring-Local',
              'SWDC-Torus', 'SA-SWDC-Torus', 'SA-SWDC-Torus-Local', 'Jellyfish',
              'SA-Jellyfish', 'SA-Jellyfish-Local']  # degree = 6
    # print(data221109.node36)
    # rescale
    nodes = [data.node900, data.node1600, data.node2500]
    for i in range(len(nodes)):
        param = nodes[i]
        ref1 = deepcopy(param[0])
        ref2 = deepcopy(param[7])
        for j in range(0, 7):
            for k in range(len(param[j])):
                param[j][k] /= ref1[k]
        for j in range(7, 17):
            for k in range(len(param[j])):
                param[j][k] /= ref2[k]
    nodes = np.array(nodes)

    font = {
        'family': 'serif',
        'weight': 'normal',
        'size': 5,
    }
    with plt.style.context(['science']):

        for idx, node_num in enumerate([900, 1600, 2500]):
            plt.figure()
            # start, end = 0, 7
            start, end = 7, 17
            length = len(labels[start:end])

            node_list = nodes[idx]

            x = np.arange(3, dtype=np.float64)
            total_width = 0.8
            width = total_width / length
            x -= (total_width - width) / 2

            for i in range(start, end):
                # print(node_list[i])
                if i == (end - start) // 2:
                    plt.bar(x + i * width, node_list[i],
                            width=width, label=labels[i], tick_label=['Avg', 'Var', 'Balance'])
                else:
                    plt.bar(x + i * width, node_list[i],
                            width=width, label=labels[i])

            plt.legend(prop={'size': 3}, loc=0)
            plt.tick_params(labelsize=5)
            plt.yscale('log')
            plt.ylabel('Cost', fontdict=font)

            plt.savefig(
                'temp/221110_figures/{:s}.jpg'.format('bar_degree6_node{:d}_log'.format(node_num)), dpi=3000)

        # ----------- ------------ -----------#
        # start, end = 0, 5
        # start, end = 5, 12
        # length = len(nodes)

        # x = np.arange(end - start, dtype=np.float64)
        # total_width = 0.8
        # width = total_width / length
        # x -= (total_width - width) / 2

        # cl = ['forestgreen', 'royalblue', 'orange']

        # for i in range(length):
        #     # print('------------', nodes[i, :, 0].flatten())
        #     if i == length // 2:
        #         plt.bar(x + i * width, nodes[i, start: end, 0].flatten(),
        #                 width=width * 0.9, label='avg', tick_label=labels[start:end], color=cl[0])
        #         plt.bar(x + i * width, nodes[i, start: end, 1].flatten(), bottom=nodes[i, start: end, 0].flatten(),
        #                 width=width * 0.9, label='var', color=cl[1])
        #         plt.bar(x + i * width, nodes[i, start: end, 2].flatten(), bottom=nodes[i, start: end, 0].flatten() + nodes[i, start: end, 1].flatten(),
        #                 width=width * 0.9, label='balance', color=cl[2])
        #     else:
        #         plt.bar(x + i * width, nodes[i, start: end, 0].flatten(),
        #                 width=width * 0.9, color=cl[0])
        #         plt.bar(x + i * width, nodes[i, start: end, 1].flatten(), bottom=nodes[i, start: end, 0].flatten(),
        #                 width=width * 0.9, color=cl[1])
        #         plt.bar(x + i * width, nodes[i, start: end, 2].flatten(), bottom=nodes[i, start: end, 0].flatten() + nodes[i, start: end, 1].flatten(),
        #                 width=width * 0.9, color=cl[2])

        # plt.legend(prop={'size': 3}, loc=0)
        # plt.tick_params(labelsize=4)
        # # plt.yscale('log')
        # plt.ylabel('Cost', fontdict=font)

        # plt.savefig(
        #     'temp/221108_figures/{:s}.jpg'.format('bar_degree6'), dpi=3000)

def draw_bar1(file_name: str):
    labels = ['2D-Torus', 'SWDC-Ring', 'SA-SWDC-Ring', 'SA-SWDC-Ring-Local',
              'Jellyfish', 'SA-Jellyfish', 'SA-Jellyfish-Local',  # degree = 4
              'Spinnaker', 'SWDC-Ring', 'SA-SWDC-Ring', 'SA-SWDC-Ring-Local',
              'SWDC-Torus', 'SA-SWDC-Torus', 'SA-SWDC-Torus-Local', 'Jellyfish',
              'SA-Jellyfish', 'SA-Jellyfish-Local']  # degree = 6
    labels4 = ['2D-Torus', 'SWDC-Ring', 'SA-SWDC-Ring', 'Jellyfish', 'SA-Jellyfish']
    labels6 = ['Spinnaker', 'SWDC-Ring', 'SA-SWDC-Ring', 'SWDC-Torus', 'SA-SWDC-Torus', 
               'Jellyfish', 'SA-Jellyfish']
    # rescale
    nodes = [data.node900, data.node1600, data.node2500]
    for i in range(len(nodes)):
        param = nodes[i]
        ref1 = deepcopy(param[0])
        ref2 = deepcopy(param[7])
        for j in range(0, 7):
            for k in range(len(param[j])):
                param[j][k] /= ref1[k]
        for j in range(7, 17):
            for k in range(len(param[j])):
                param[j][k] /= ref2[k]
    nodes4 = [None] * 3
    nodes6 = [None] * 3
    for i in range(3):
        nodes4[i] = [nodes[i][0], nodes[i][1], nodes[i][2], nodes[i][4], nodes[i][5]]
        nodes6[i] = [nodes[i][7], nodes[i][8], nodes[i][9], nodes[i][11], nodes[i][12], nodes[i][14], nodes[i][15]]
    nodes4 = np.array(nodes4)
    nodes6 = np.array(nodes6)

    font = {
        'family': 'serif',
        'weight': 'normal',
        'size': 10,
    }
    with plt.style.context(['science']):

        for idx, node_num in enumerate([900, 1600, 2500]):
            plt.figure(figsize=(3.2, 2.8))
            # plt.set_size_inches(3.2, 2.2)
            
            length = len(labels4)
            node_list = nodes4[idx]

            x = np.arange(3, dtype=np.float64)
            total_width = 0.8
            width = total_width / length
            x -= (total_width - width) / 2

            for i in range(length):
                if i == length // 2:
                    plt.bar(x + i * width, node_list[i],
                            width=width, label=labels4[i], tick_label=['$SP_{avg}$', '$SP_{var}$', '$PB$'])
                else:
                    plt.bar(x + i * width, node_list[i],
                            width=width, label=labels4[i])

            plt.legend(prop={'size': 8}, loc=0, bbox_to_anchor=(1.0, 0.66), ncol=1)
            plt.tick_params(labelsize=8)
            plt.yscale('log')
            plt.ylabel('Cost', fontdict=font)

            plt.savefig(
                'temp/221110_figures/new/{:s}.pdf'.format('bar_degree4_node{:d}_log'.format(node_num)), dpi=5000)
            
            plt.figure(figsize=(3.2, 2.8))
            
            length = len(labels6)
            node_list = nodes6[idx]

            x = np.arange(3, dtype=np.float64)
            total_width = 0.8
            width = total_width / length
            x -= (total_width - width) / 2

            for i in range(length):
                if i == length // 2:
                    plt.bar(x + i * width, node_list[i],
                            width=width, label=labels6[i], tick_label=['$SP_{avg}$', '$SP_{var}$', '$PB$'])
                else:
                    plt.bar(x + i * width, node_list[i],
                            width=width, label=labels6[i])

            plt.legend(prop={'size': 8}, loc=0, bbox_to_anchor=(1.0, 0.75), ncol=1)
            plt.tick_params(labelsize=8)
            plt.yscale('log')
            plt.ylabel('Cost', fontdict=font)

            plt.savefig(
                'temp/221110_figures/new/{:s}.pdf'.format('bar_degree6_node{:d}_log'.format(node_num)), dpi=5000)

if __name__ == '__main__':

    draw_bar1("123")
