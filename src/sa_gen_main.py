from main import parallel_run, run_base
from itertools import product

def sa_gen():
    # sa_swdc_ring
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
    cmds = []
    for para, name in sa_list:
        cmds.append('bin/sa_gen {:s} > temp/sa_gen/{:s}.txt'.format(para, name))
        print(cmds[-1])
    parallel_run(run_base, cmds, pool_size=len(sa_list))


if __name__ == '__main__':
    sa_gen()  #21346