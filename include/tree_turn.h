#ifndef __TREE_TURN_H__
#define __TREE_TURN_H__ 1

#include "topology.h"

typedef struct
{
    int *x;
    int *y;
} TreeCoordinate;

TreeCoordinate *gen_tree_coordinate(const GraphAdjList *G, int root, int type); // type 0 - DFS; 1 - BFS

void delete_tc(TreeCoordinate *tc);

int is_lu(int x1, int y1, int x2, int y2);
int is_l(int x1, int y1, int x2, int y2);
int is_ld(int x1, int y1, int x2, int y2);
int is_ru(int x1, int y1, int x2, int y2);
int is_r(int x1, int y1, int x2, int y2);
int is_rd(int x1, int y1, int x2, int y2);

int check_tree_trun(int x1, int y1, int x2, int y2, int x3, int y3);

void tree_turn_bfs_shortest_path_algorithm_parallel(const GraphAdjList *G, const TreeCoordinate *tc, unsigned int **dist, int **path);

void tree_turn_floyd_algorithm_parallel(int num_nodes, const TreeCoordinate *tc, unsigned int **dist, int **path);

void tree_turn_check_path(int num_nodes, int **path, TreeCoordinate *tc);

int *statistic_path(const GraphAdjList *G, const TreeCoordinate *tc);

#endif