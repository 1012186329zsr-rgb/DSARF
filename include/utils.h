#ifndef __UTILS_H__
#define __UTILS_H__ 1

#include "topology.h"

GraphAdjList *construct_graph_adjlist_from_file(char *file_name);

void print_graph_adjlist(GraphAdjList *G);

void write_graph_adjlist_to_file(GraphAdjList *G, char *file_name);

void write_path_log_to_file(const GraphAdjList *G, unsigned int *path_log, char *file_name);

void delete_graph(GraphAdjList *G);

void check_graph_adjlist(GraphAdjList *G);

int dst_torus(int *cord1, int *cord2, int ary, int cube);

void check_graph_dst_ring(GraphAdjList *G, int dst_threshold);

void check_graph_dst_torus(GraphAdjList *G, int ary, int cube, int dst_threshold);

int check_graph_matrix(GraphMatrix *G);

double variance(int length, unsigned int *array);

int check_graph_connectivity(GraphAdjList *G);

void print_matrix(int **matrix, int num, char *name);

void transpose(int nums, int **matrix);

#endif