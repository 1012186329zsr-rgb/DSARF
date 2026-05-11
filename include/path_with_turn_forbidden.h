#ifndef __PATH_WITH_TURN_FORBIDDEN_H__
#define __PATH_WITH_TURN_FORBIDDEN_H__ 1

#include "topology.h"
#include "tree_turn.h"
#include "route_sim_anytopo.h"

typedef struct path_node PathNode;
struct path_node
{
    int id;
    int depth;
    PathNode *father;
    int children_num;
};

void trun_model_dijkstra_parallel(const GraphAdjList *G, const TreeCoordinate *tc, unsigned int **dist, int ***path, int (*turn_check)(int, int, int, int, int, int));

void turn_model_check_path(int num_nodes, int ***path, TreeCoordinate *tc, int (*turn_check)(int, int, int, int, int, int));

void turn_model_initial_path_dist(const GraphAdjList *G, unsigned int ***dist, int ****path);

void turn_model_delete_path_dist(const GraphAdjList *G, unsigned int **dist, int ***path);

double turn_model_cost(const GraphAdjList *G, unsigned int **dist, int ***path, unsigned int *history_path_use_array, int update_en);

int *root_selection(const GraphAdjList *G, int (*turn_check)(int, int, int, int, int, int), int root_num);

void trun_model_bfs(const GraphAdjList *G, const TreeCoordinate *tc, int root, unsigned int *dist, int **path, int (*turn_check)(int, int, int, int, int, int));

void trun_model_bfs_parallel(const GraphAdjList *G, const TreeCoordinate *tc, unsigned int **dist, int ***path, int (*turn_check)(int, int, int, int, int, int));

void trun_model_bfs_new(const GraphAdjList *G, TreeCoordinate **tc, RouterList *R, int (*turn_check)(int, int, int, int, int, int));

#endif