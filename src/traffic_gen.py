import numpy as np
import os
import math
import random
from itertools import product


def local_random(num_nodes: int, lam: int, send_dst_num: int):
    random.seed(0)
    np.random.seed(0)
    res = np.zeros((num_nodes, num_nodes), dtype=np.int32)
    dimx = math.floor(math.sqrt(num_nodes))
    while int(num_nodes / dimx) * dimx != num_nodes:
        dimx -= 1
    dimy = num_nodes // dimx
    print('{:d} = {:d} X {:d}'.format(num_nodes, dimx, dimy))
    for i in range(num_nodes):
        dxi = i % dimx
        dyi = i // dimx
        poisson = np.random.poisson(lam=lam, size=send_dst_num)
        for dst in poisson:
            dx = random.randint(0, min(dst, dimx))
            dy = min(dimy, dst - dx)
            x, y = (dxi + dx) % dimx, (dyi + dy) % dimy
            idx = x + y * dimx
            res[i, idx] = 100  # packet num
    print('Done!')
    return res

def bit_shuffle(num_nodes: int, mode=0):
    random.seed(0)
    np.random.seed(0)
    res = np.zeros((num_nodes, num_nodes), dtype=np.int32)
    bit_num = math.ceil(math.log2(num_nodes - 1))
    maxb, max_idx = [], num_nodes - 1
    while max_idx > 0:
        maxb.append(max_idx % 2)
        max_idx //= 2
    for i in range(num_nodes):
        ib = [] # LSB ~ MSB
        ii = i
        while ii > 0:
            ib.append(ii % 2)
            ii //= 2
        assert(len(ib) <= bit_num)
        ib.extend([0] * (bit_num - len(ib)))
        if mode == 0:  # bit shuffle
            ib = [ib[-1], *ib[:-1]]
        elif mode == 1: # bit complement
            ib = [1 if bit == 0 else 0 for bit in ib]
        elif mode == 2: # bit reverse
            ib.reverse()
        else:
            raise NotImplementedError
        dst = 0
        for j in range(1, bit_num + 1):
            dst *= 2
            dst += ib[bit_num - j]
        if dst >= num_nodes:
            for j in range(0, bit_num):
                ib[j] = ib[j] * maxb[j]
            dst = 0
            for j in range(1, bit_num + 1):
                dst *= 2
                dst += ib[bit_num - j]
        res[i, dst] = 1000
    return res


if __name__ == '__main__':
    node_num = [20, 40, 60, 80, 100, 200, 400, 600, 800, 1000,
                1200, 1400, 1600, 1800]
    send_probability = [100, 500, 1000]
    for i, j in product(node_num, send_probability):
        res = local_random(num_nodes=i, lam=64, send_dst_num=max(1, (j * i // 10000)))
        np.savetxt('temp/traffic_table/local_random_lam/traffic_{:d}_{:d}.csv'.format(i, j), res, fmt='%d', delimiter=',')
    
    # for i in node_num:
    #     res = bit_shuffle(num_nodes=i, mode=0)
    #     np.savetxt('temp/traffic_table/bit_/traffic_{:d}_.csv'.format(i), res, fmt='%d', delimiter=',')
    