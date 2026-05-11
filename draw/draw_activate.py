import numpy as np
import math
import matplotlib.pyplot as plt
import matplotlib.style as style

plt.rcParams['font.size'] = 6

font = {
        'family': 'serif',
        'weight': 'normal',
        'size': 8,
    }

with plt.style.context(['science', 'ieee']):
    plt.rcParams['font.size'] = 6
    fig, ax = plt.subplots(2, 4)
    fig.set_size_inches(4, 2)
    plt.subplots_adjust(left=None,bottom=None,right=None,top=None,wspace=0.3,hspace=0.3)

    x = np.linspace(-10, 10, 100000) 
    
    y1 = np.array(list(map(lambda x: 1 if x == 0 else 0, x)))
    
    # norm
    u, sig = 0, 1
    y2 = np.exp(-(x - u) ** 2 / (2 * sig ** 2)) / (math.sqrt(2*math.pi)*sig)
    
    # 
    y3 = np.array(list(map(lambda x: 1 if (-0.5 <= x <= 0.5) else 0, x)))
    
    y4 = np.array(list(map(lambda x: x + 1 if x < 0 else -x + 1, x)))
    
    sigmoid = lambda x: 1 / (1 + np.exp(-x))
    y5 = sigmoid(x) * (1 - sigmoid(x))
    
    y6 = 1 - np.tanh(x) ** 2
    
    y7 = np.array(list(map(lambda x: 1 / ((1 + np.abs(x) ** 2)), x)))
    
    y8 = 1 / (1 + (x ** 2))
    
    ax[0, 0].plot(x, np.zeros_like(x), label='$\delta(t)$')
    ax[0, 0].set_ylim(0, 1.5)
    ax[0, 0].set_xlim(-5, 5)
    ax[0, 1].plot(x, y2, label='$Gaussian$')
    ax[0, 1].set_ylim(0, 1.5)
    ax[0, 1].set_xlim(-5, 5)
    ax[0, 2].plot(x, y3, label='$Rectangle$')
    ax[0, 2].set_ylim(0, 1.5)
    ax[0, 2].set_xlim(-5, 5)
    ax[0, 3].plot(x, y4, label='$Triangle$')
    ax[0, 3].set_ylim(0, 1.5)
    ax[0, 3].set_xlim(-5, 5)
    ax[1, 0].plot(x, y5, label='$Sigmoid^{\prime}$')
    ax[1, 0].set_ylim(0, 1.5)
    ax[1, 0].set_xlim(-5, 5)
    ax[1, 1].plot(x, y6, label='$Tanh^{\prime}$')
    ax[1, 1].set_ylim(0, 1.5)
    ax[1, 1].set_xlim(-5, 5)
    ax[1, 2].plot(x, y7, label='$Softsign^{\prime}$')
    ax[1, 2].set_ylim(0, 1.5)
    ax[1, 2].set_xlim(-5, 5)
    ax[1, 3].plot(x, y8, label='$Arctan^{\prime}$')
    ax[1, 3].set_ylim(0, 1.5)
    ax[1, 3].set_xlim(-5, 5)
    
    for i in range(2):
        for j in range(4):
            # ax[i, j].autoscale(tight=True)
            ax[i, j].tick_params(labelsize=5)
            ax[i, j].legend(prop={'size': 5}, loc=0)

    # plt.legend(prop={'size': 6}, loc=0)
    
    # plt.tick_params(labelsize=6)
    # plt.xlabel('Distance', fontdict=font)
    # plt.ylabel('Cost', fontdict=font)
    # plt.xlim((0, 90))

    fig.savefig('act.pdf', dpi=5000)