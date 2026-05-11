#ifndef __SA_H__
#define __SA_H__ 1

#include "topology.h"

#define SA_T_INITIAL 10000     // initial temperature
#define SA_T_STOP 0.0          // stop temperature
#define SA_NOT_TAKEN_MAX 10000 // max value of not taken
#define SA_ALPHA 0.9999        // temperature decay rate

GraphAdjList *sa_swdc_torus_gen(int ary, int cube, int rand_degrees, int dst_threshold, unsigned int seed);

GraphAdjList *sa_jellyfish_gen(int num_nodes, int degrees, int dst_threshold, unsigned int seed);

void sa_cost(GraphAdjList *G, int ***ref_path, unsigned int ***dist, int ***path, double *avg_shortest_path, double *shortest_path_var, double *path_load_balance);

void sa_cost_bfs(const GraphAdjList *G, unsigned int ***dist, int ***path, double *avg_shortest_path, double *shortest_path_var, double *path_load_balance);

void sa_cost_step(GraphAdjList *G, int ***ref_path, unsigned int ***dist, int ***path, double *avg_shortest_path, double *shortest_path_var, double *path_load_balance);

void sa_cost_bfs_step(const GraphAdjList *G, unsigned int ***dist, int ***path, double *avg_shortest_path, double *shortest_path_var, double *path_load_balance);

#endif