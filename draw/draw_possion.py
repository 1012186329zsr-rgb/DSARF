import numpy as np
import scipy.stats as stats
import matplotlib.pyplot as plt
import matplotlib.style as style

# 绘图配置
style.use('seaborn-bright')

plt.rcParams['font.size'] = 6

font = {
        'family': 'serif',
        'weight': 'normal',
        'size': 8,
    }

with plt.style.context(['science', 'ieee']):

    # 一段时间内发生的次数
    data = np.arange(90)

    # PMF 绘制泊松分布的概率密度函数
    plt.plot(data, stats.poisson.pmf(data, mu=8), label='$\lambda = 8$')
    plt.bar(data, stats.poisson.pmf(data, mu=8), alpha=.5)

    # PMF 绘制泊松分布的概率密度函数
    plt.plot(data, stats.poisson.pmf(data, mu=16), label='$\lambda = 16$')
    plt.bar(data, stats.poisson.pmf(data, mu=16), alpha=.5)

    # PMF 绘制泊松分布的概率密度函数
    plt.plot(data, stats.poisson.pmf(data, mu=32), label='$\lambda = 32$')
    plt.bar(data, stats.poisson.pmf(data, mu=32), alpha=.5)

    plt.plot(data, stats.poisson.pmf(data, mu=64), label='$\lambda = 64$')
    plt.bar(data, stats.poisson.pmf(data, mu=64), alpha=.5)

    plt.legend(prop={'size': 6}, loc=0)
    
    plt.tick_params(labelsize=6)
    plt.xlabel('Distance', fontdict=font)
    plt.ylabel('Cost', fontdict=font)
    plt.xlim((0, 90))

    plt.savefig('temp/possion.pdf', dpi=5000)

    # print('p(x<8)时的概率：{}'.format(stats.poisson.cdf(k=8, mu=15)))
    # print('p(8<x<20)时的概率：{}'.format(stats.poisson.cdf(k=20, mu=15) - stats.poisson.cdf(k=8, mu=15)))