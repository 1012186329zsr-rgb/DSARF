#ifndef __TOPO_GEN_H__
#define __TOPO_GEN_H__ 1

#include "topology.h"

GraphAdjList *torus_gen(int ary, int cube);

GraphAdjList *swdc_torus_gen(int ary, int cube, int rand_degrees, unsigned int seed);

GraphAdjList *swdc_torus_local_gen(int ary, int cube, int rand_degrees, int dst_threshold, unsigned int seed);

GraphAdjList *jellyfish_gen(int num_nodes, int degrees, unsigned int seed);

GraphAdjList *jellyfish_local_gen(int num_nodes, int degrees, int dst_threshold, unsigned int seed);

GraphAdjList *spinakker_gen(int ary);

#endif