#ifndef __DFS_H__
#define __DFS_H__ 1

#include "topology.h"

void dfs(GraphAdjList *G, int *visit, int *stack);

void bfs(const GraphAdjList *G, int root, unsigned int *dist, int *path);

void bfs_initial_path_dist(const GraphAdjList *G, unsigned int ***dist_p, int ***path_p);

void bfs_delete_path_dist(unsigned int **dist, int **path);

void bfs_shortest_path_algorithm(const GraphAdjList *G, unsigned int **dist, int **path);

void bfs_shortest_path_algorithm_parallel(const GraphAdjList *G, unsigned int **dist, int **path);

#endif
