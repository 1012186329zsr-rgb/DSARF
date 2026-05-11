import matplotlib.pyplot as plt
import numpy as np
import matplotlib.ticker as mtick
from itertools import product
from parse_path_log import parse_path_log
import seaborn as sns
import pandas as pd


def draw_path_hist(log_name, figure_name):
    with plt.style.context(['science', 'ieee']):
        plt.rcParams['font.size'] = 5
        fig, ax1 = plt.subplots()
        fig.set_size_inches(3.2, 2.4)
        
        ax1.hist(np.array(parse_path_log(log_name[0]), dtype=np.int32), bins=50, alpha=0.5, label='Deterministic')
        # plt.hist(np.array(parse_path_log(log_name[1]), dtype=np.int32), bins=50, alpha=0.5, label='2222222')
        # plt.hist(np.array(parse_path_log(log_name[2]), dtype=np.int32), bins=50, alpha=0.5, label='3333333')
        ax1.hist(np.array(parse_path_log(log_name[3]), dtype=np.int32), bins=50, alpha=0.5, label='Non-local congestion aware \& non-minimal')
        
        font = {
            'family': 'serif',
            'weight': 'normal',
            'size': 8,
        }
        
        # ax1.set_ylim(0, 6000)
        ax1.set_yscale('log')
        plt.legend(prop={'size': 6})
        ax1.set_xlabel('Path Load', fontdict=font)
        ax1.set_ylabel('Number', fontdict=font)
        plt.yticks(size=6)
        plt.xticks(size=6)
        
        
        ax2 = ax1.twinx()
        ax2.set_ylabel('Average Packet Delivery Latency/cycles', fontdict=font)
        # ax2.set_ylim(0, 6000)
        # ax2.set_yscale('log')
        
        plt.savefig(figure_name, dpi=5000)


if __name__ == '__main__':
    file_list = []
    for i in range(4):
        file_list.append('temp/gpt_xl_t1000/path_log/{:d}.txt'.format(i))
    draw_path_hist(file_list, 'temp/gpt_xl_t1000/gpt_xl_t1000_path_load.pdf')
