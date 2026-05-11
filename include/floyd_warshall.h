// Floyd-Warshall Algorithm
// Reference https://blog.csdn.net/ThinPikachu/article/details/123678745?spm=1001.2101.3001.6650.3&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-3-123678745-blog-60875818.pc_relevant_3mothn_strategy_recovery&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-3-123678745-blog-60875818.pc_relevant_3mothn_strategy_recovery&utm_relevant_index=6
#ifndef __FLOYD_WARSHALL_H__
#define __FLOYD_WARSHALL_H__ 1

#include "topology.h"

// works for unsigned int32
#ifndef INF
#define INF 0x7f7f7f7f
#endif

void floyd_algorithm(int num_nodes, unsigned int **dist, int **path);

void floyd_algorithm_step(int num_nodes, unsigned int **dist, int **path, int *node_list, int **matrix);

void floyd_algorithm_parallel(int num_nodes, unsigned int **dist, int **path);

void floyd_initial_path_dist(const GraphAdjList *G, unsigned int ***dist_p, int ***path_p);

void floyd_delete_path_dist(unsigned int **dist, int **path);

double floyd_average_shortest_path_length(int num_nodes, unsigned int **dist);

double floyd_shortest_path_variance(int num_nodes, unsigned int **dist);

void floyd_check_path(const GraphAdjList *G, int **path);

void floyd_check_dist(const GraphAdjList *G, unsigned int **dist);

unsigned int *floyd_initial_path_load_balance(const GraphAdjList *G, int **path);

double floyd_path_load_balance(int num_edges, unsigned int *array);

#endif