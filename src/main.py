import os
from multiprocessing import Pool, freeze_support, cpu_count
from itertools import product
import time


def parallel_run(func, args: list, pool_size: int):
    print("Parallel run START ...")

    freeze_support()
    pool = Pool(pool_size)
    for arg in args:
        # assert isinstance(arg, tuple)
        pool.apply_async(func=func, args=(arg,))
    pool.close()
    pool.join()
    pool.terminate()
    print('Parallel run DONE!')


def run_base(cmd: str):
    old_time = time.time()
    res = os.system(cmd)
    print('**** End of $$ {:s} $$, time = {:.4f}, output = {}\n'.format(
        cmd, time.time() - old_time, res))


##### SA #####
def run_sa(num, degree):
    old_time = time.time()
    res = os.system(
        'bin/topology {:d} {:d} > temp/sa_bfs_jellyfish_{:d}_{:d}_0_log.txt'.format(num, degree, num, degree))
    print('**** End of ({:d}, {:d}), time = {:.4f}, output = {}\n'.format(
        num, degree, time.time() - old_time, res))


def run_topo_main():
    topos = [
        # (0, 2, 0),  # torus 4
        # (0, 1, 2),  # swdc_ring 4
        # (4, 1, 2),  # sa_swdc_ring 4
        # (1, 4),  # jellyfish 4
        (3, 4),  # sa_jellyfish 4
        # (2, ),  # spinnaker 6
        # (0, 1, 4),  # swdc_ring 6
        # (4, 1, 4),  # sa_swdc_ring 6
        # (0, 2, 2),  # swdc_torus 6
        # (4, 2, 2),  # sa_swdc_torus 6
        # (1, 6),  # jellyfish 6
        (3, 6),  # sa_jellyfish 6
    ]
    args = []
    for topo in topos:
        for j in [6, 8, 30, 40, 50]:
            ### swdc_torus ###
            if topo[0] == 0:
                if topo[1] == 1:
                    args.append((0, j*j, *topo[1:]))  # swdc_ring
                elif topo[1] == 2:
                    args.append((0, j, *topo[1:]))  # torus, swdc_torus
                else:
                    raise NotImplementedError
            ### jellyfish ###
            elif topo[0] == 1:
                args.append((1, j*j, *topo[1:]))  # jellyfish
            ### spinnaker ###
            elif topo[0] == 2:
                args.append((2, j, *topo[1:]))  # spinnaker
            ### sa_jellyfish ###
            elif topo[0] == 3:
                args.append((3, j*j, *topo[1:], j*j))  # sa_jellyfish
                if j > 10:
                    # sa_jellyfish_local
                    args.append((3, j*j, *topo[1:], int(0.1*j*j)))
            ### sa_swdc ###
            elif topo[0] == 4:
                if topo[1] == 1:
                    args.append((4, j*j, *topo[1:], j*j))  # sa_swdc_ring
                    if j > 10:
                        # sa_swdc_ring_local
                        args.append((4, j*j, *topo[1:], int(0.1*j*j)))
                elif topo[1] == 2:
                    args.append((4, j, *topo[1:], j*j))  # sa_swdc_torus
                    if j > 10:
                        # sa_swdc_torus_local
                        args.append((4, j, *topo[1:], int(j/3)))
            else:
                raise NotImplementedError
    cmds = []
    for idx, i in enumerate(args):
        temp = 'bin/topo221110_longlong '
        for j in i:
            temp += '{:d} '.format(j)
        temp += '> temp/221110_log/{:d}_longlong.log'.format(idx)
        cmds.append(temp)
        # print(temp)
    # print(cmds)
    parallel_run(run_base, args=cmds, pool_size=5)


##### Route Sim #####
def run_route_sim_anytopo(file_name: str):
    old_time = time.time()
    print('-------' + file_name)
    res = os.system(
        'bin/main {:s} > temp/221102_sim_bandwidth/{:s}_log.txt'.format(file_name, file_name))
    print('**** End of {:s}, time = {:.4f}, output = {}\n'.format(
        file_name, time.time() - old_time, res))


def run_route_sim_anytopo_main():
    topo_list = [
        'torus_{:d}_4',  'swdc_ring_{:d}_4',
        'swdc_ring_{:d}_6', 'sa_swdc_ring_{:d}_4',
        'sa_swdc_ring_{:d}_6', 'jellyfish_{:d}_4',
        'jellyfish_{:d}_6',  'sa_jellyfish_{:d}_4',
        'sa_jellyfish_{:d}_6', 'spinnaker_{:d}_6',
        'swdc_torus_{:d}_6', 'sa_swdc_torus_{:d}_6'
    ]
    node_num = [64, 900, 1600, 2500]
    file_name = [i.format(j) for i, j in product(topo_list, node_num)]
    print(file_name)
    parallel_run(run_route_sim_anytopo, file_name, pool_size=20)

##### Deadlock Topo #####


def run_deadlock_topo():
    node_num = [20, 40, 60, 80, 100, 200, 400, 600, 800, 1000,
                1200, 1400, 1600, 1800, 2000, 2200, 2400, 2600, 2800, 3000]
    degree = [4, 6, 8]
    for i, j in product(node_num, degree):
        os.system(
            'bin/main {:d} {:d}'.format(i, j))


def run_deadlock_sim():
    node_num = [20, 40, 60, 80, 100, 200, 400, 600, 800, 1000,
                1200, 1400, 1600, 1800, 2000, 2200, 2400, 2600, 2800, 3000]
    degree = [4, 6, 8]
    cmds = []
    # cmds = ["bin/deadlock_wo_rootselect temp/deadlock_free/topo/jellyfish_{:d}_{:d}.txt 10000 {:d} > temp/deadlock_free/log/{:d}.log".format(
    #     i, j, k, idx) for idx, (i, j, k) in enumerate(product(node_num, degree, [0, 1]))]
    for idx, (i, j, k) in enumerate(product(node_num, degree, [0, 1])):
        if i in [1400, 1600, 1800]:
            cmds.append("bin/deadlock_wo_rootselect temp/deadlock_free/topo/jellyfish_{:d}_{:d}.txt 10000 {:d} > temp/deadlock_free/log/{:d}.log".format(
                i, j, k, idx))
    for idx, (i, j) in enumerate(product(node_num, degree)):
        if i in [1400, 1600, 1800]:
            cmds.append("bin/deadlock_w_rootselect temp/deadlock_free/topo/jellyfish_{:d}_{:d}.txt 10000 1 1 > temp/deadlock_free/log/{:d}.log".format(
                i, j, idx + 1000))
    # cmds = ["bin/deadlock_w_rootselect temp/deadlock_free/topo/jellyfish_{:d}_{:d}.txt 10000 1 1 > temp/deadlock_free/log/{:d}.log".format(
    #     i, j, idx + 1000) for idx, (i, j) in enumerate(product(node_num, degree))]
    print(cmds)
    parallel_run(run_base, cmds, pool_size=1)

def run_deadlock_mvc_sim():
    node_num = [20, 40, 60, 80, 100, 200, 400, 600, 800, 1000,
                1200, 1400, 1600, 1800]
    degree = [4, 6, 8]
    cmds = ["bin/deadlock_2vc3 temp/deadlock_free/topo/jellyfish_{:d}_{:d}.txt 10000 1 {:d} > temp/deadlock_free/2vc3_log/{:d}.log".format(
        i, j, k, idx) for idx, (i, j, k) in enumerate(product(node_num, degree, [0, 1, 2]))]
    print(cmds)
    parallel_run(run_base, cmds, pool_size=1)

def run_adaptive_sim():
    node_num = [20, 40, 60, 80, 100, 200, 400, 600, 800, 1000,
                1200, 1400, 1600, 1800]
    degree = [4, 6, 8]
    send_probability = [100, 500, 1000]
    diversity_load = [(-1, 0), (0, 0), (0, 1), (1, 1)]
    cmds = ["bin/adaptive_2vc3 temp/deadlock_free/topo/jellyfish_{:d}_{:d}.txt 0 1 0 {:d} 100 2 {:d} {:d} > temp/adaptive/2vc3/{:d}.log".format(
        i, j, k, m[0], m[1], idx) for idx, (i, j, k, m) in enumerate(product(node_num, degree, send_probability, diversity_load))]
    cmd_new = []
    for i in range(len(cmds)):
        if (468 <= i <= 502):
        # if (i in [456, 457, 458, 466]) or (468 <= i <= 502):
            cmd_new.append(cmds[i])
    print(cmd_new)
    parallel_run(run_base, cmd_new, pool_size=8)

def run_adaptive_non_local_sim():
    node_num = [20, 40, 60, 80, 100, 200, 400, 600, 800, 1000,
                1200, 1400, 1600, 1800]
    degree = [4, 6, 8]
    send_probability = [100, 500, 1000]
    diversity_load = [(0, 2), (1, 2)]
    cmds = ["bin/adaptive_non_local_2vc3 temp/deadlock_free/topo/jellyfish_{:d}_{:d}.txt 0 1 0 {:d} 100 2 {:d} {:d} > temp/adaptive/2vc3_non_local/{:d}.log".format(
        i, j, k, m[0], m[1], idx) for idx, (i, j, k, m) in enumerate(product(node_num, degree, send_probability, diversity_load))]
    print(cmds)
    parallel_run(run_base, cmds, pool_size=5)

def run_gpt_xl_t1000():
    route_lut_mode = [1] # 0 - L-turn; 1 - Tree-turn;
    root_selection = [2] # 0 - random same; 1 - random independently; 2 - random optimally
    diversity_load = [(-1, 0), (0, 0), (0, 1), (1, 2)]
    cmds = ["bin/gpt_xl temp/deadlock_free/topo/jellyfish_1400_4.txt -1 {:d} 0 100 100 {:d} {:d} {:d} temp/gpt_xl_t1000/path_log/{:d}.txt /home/common/sch/autospliter/gpt_xl_t1000_processed_div1000.csv 1400 > temp/gpt_xl_t1000/log/{:d}.log".format(
        i, j, k[0], k[1], idx, idx) for idx, (i, j, k) in enumerate(product(route_lut_mode, root_selection, diversity_load))]
    print(cmds)
    parallel_run(run_base, cmds, pool_size=4)
    
def run_traffic_local():
    node_num = [20, 40, 60, 80, 100, 200, 400, 600, 800, 1000,
                1200, 1400, 1600]
    degree = [4, 6, 8]
    send_probability = [100, 500, 1000]
    diversity_load = [(-1, 0), (0, 0), (0, 1), (1, 2)]
    cmds = ["bin/gpt_xl temp/deadlock_free/topo/jellyfish_{:d}_{:d}.txt -1 1 0 100 100 2 {:d} {:d} temp/local_random_lam64/path_log/{:d}.txt temp/traffic_table/local_random_lam64/traffic_{:d}_{:d}.csv {:d} > temp/local_random_lam64/log/{:d}.log".format(
        n, d, k[0], k[1], idx, n, s, n, idx) for idx, (n, d, k, s) in enumerate(product(node_num, degree, diversity_load, send_probability))]
    print(cmds)
    parallel_run(run_base, cmds, pool_size=8)

if __name__ == '__main__':

    # run_sa_main()
    # run_route_sim_anytopo_main()
    # run_topo_main()
    # run_deadlock_topo()
    # run_deadlock_sim()  # 15149 18641 14548
    # run_deadlock_mvc_sim() #34895
    # ---run_adaptive_sim() #29294
    # run_adaptive_non_local_sim() #24342
    # run_gpt_xl_t1000() # 34311
    run_traffic_local()  # 55374
