#ifndef __GRAPH_ANALYSIS_H__
#define __GRAPH_ANALYSIS_H__ 1
#include "topology.h"

unsigned int diameter(unsigned int **dist, int num_nodes);

void local_cluster_coefficient(int idx, int **matrix, int num_nodes, int *degree, int *cnt);

double average_cluster_coefficient(GraphAdjList *G);

void between_centrality(int **path, int num_nodes, int **centrality_list);

#endif