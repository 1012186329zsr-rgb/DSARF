#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "graph_analysis.h"
#include "floyd_warshall.h"

unsigned int diameter(unsigned int **dist, int num_nodes)
{
    unsigned int max_dist = 0;
    for (int i = 0; i < num_nodes; ++i)
    {
        for (int j = 0; j < num_nodes; ++j)
        {
            if (dist[i][j] > max_dist)
                max_dist = dist[i][j];
        }
    }
    return max_dist;
}

void local_cluster_coefficient(int idx, int **matrix, int num_nodes, int *degree, int *cnt)
{
    assert(matrix != NULL);
    (*degree) = 0;
    int *friends = (int *)calloc(num_nodes, sizeof(int));
    assert(friends != NULL);
    for (int i = 0; i < num_nodes; ++i)
    {
        if (matrix[idx][i] == 1)
            friends[(*degree)++] = i;
    }
    (*cnt) = 0;
    for (int i = 0; i < (*degree); ++i)
    {
        for (int j = i + 1; j < (*degree); ++j)
        {
            assert(matrix[i][j] == matrix[j][i]);
            if (matrix[i][j] == 1)
                (*cnt)++;
        }
    }
    free(friends);
    friends = NULL;
}

double average_cluster_coefficient(GraphAdjList *G)
{
    int total_cnt = 0;
    int total_degree = 0;
    int degree, cnt;
    int **matrix = (int **)calloc(G->num_nodes, sizeof(int *));
    assert(matrix != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        matrix[i] = (int *)calloc(G->num_nodes, sizeof(int));
        assert(matrix[i] != NULL);
        for (int j = 0; j < G->degree; ++j)
        {
            if (G->adj_list[i][j] != INF)
                matrix[i][G->adj_list[i][j]] = 1;
        }
    }
    for (int i = 0; i < G->num_nodes; ++i)
    {
        local_cluster_coefficient(i, matrix, G->num_nodes, &degree, &cnt);
        total_cnt += cnt;
        total_degree += degree * (degree - 1);
    }
    for (int i = 0; i < G->num_nodes; ++i)
    {
        free(matrix[i]);
        matrix[i] = NULL;
    }
    free(matrix);
    matrix = NULL;
    return 2. * (double)total_cnt / (double)total_degree;
}

void between_centrality(int **path, int num_nodes, int **centrality_list)
{
    assert((*centrality_list) == NULL && path != NULL);
    (*centrality_list) = (int *)calloc(num_nodes, sizeof(int));
    assert((*centrality_list) != NULL);
    for (int i = 0; i < num_nodes; ++i)
    {
        for (int j = 0; j < num_nodes; ++j)
        {
            (*centrality_list)[path[i][j]]++;
        }
    }
}