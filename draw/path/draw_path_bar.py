import matplotlib.pyplot as plt
import numpy as np
import matplotlib.ticker as mtick
from itertools import product


def main():
    
    font = {
        'family': 'serif',
        'weight': 'normal',
        'size': 8,
    }
    
    size = 2
    x = np.arange(size)
    # a = [3521423, 678.207463]
    # b = [2735728, 527.324684]
    # c = [2427895, 466.235832]
    # d = [1731088, 338.942354]
    a = [3521423, 2735728, 2427895, 1731088]
    b = [678.207463, 527.324684, 466.235832, 338.942354]
    
    with plt.style.context(['science', 'ieee']):
        plt.rcParams['font.size'] = 5
        fig, ax1 = plt.subplots()
        fig.set_size_inches(3.2, 2.4)
        x = np.arange(len(a))
        
        ax1.set_ylabel('Simulation Latency/cycles', fontsize=8)
        l1 = ax1.bar(x-0.1, a, width=0.2, label='Simulation Latency', color='b')
        
        ax2 = ax1.twinx()
        ax2.set_ylabel('Average Packet Delivery Latency/cycles', fontsize=8)
        l2 = ax2.bar(x+0.1, b, width=0.2, label='Average Packet Delivery Latency', color='r')
        ax2.set_ylim(0, 1000)
        
        g = [l1, l2]
        f = [l.get_label() for l in g]
        plt.legend(g,f, prop={'size': 6})

        ax1.set_xticks(x)
        ax1.set_xticklabels(['Deterministic', 'Non-deterministic\n\& minimal ', 'Local congestion\naware \& minimal', 'Non-local congestion\naware \& non-minimal'], 
                            rotation=0, fontsize=6)
        plt.tick_params(labelsize=5)
        fig.savefig('temp/gpt_xl_t1000/gpt_xl_t1000_cycle_apdl.pdf', dpi=5000)
        

if __name__ == '__main__':
    main()