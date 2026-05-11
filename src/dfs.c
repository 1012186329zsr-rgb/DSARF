#include "dfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "floyd_warshall.h"
#include "omp.h"

void dfs(GraphAdjList *G, int *visit, int *stack)
{
    // len(visit) == G->num_nodes
    assert(visit != NULL);
    memset(visit, 0, G->num_nodes * sizeof(int));
    int start_ptr = 0;
    assert(stack != NULL);
    // push root in stack
    stack[start_ptr++] = 0;
    visit[stack[start_ptr - 1]] = 1;
    // start DFS
    int this_node = 0;
    while (start_ptr > 0)
    {
        this_node = stack[--start_ptr];
        // assert(visit[this_node] = 1);
        for (int i = 0; i < G->degree; ++i)
        {
            if (G->adj_list[this_node][i] != INF && visit[G->adj_list[this_node][i]] == 0)
            {
                stack[start_ptr++] = G->adj_list[this_node][i];
                visit[G->adj_list[this_node][i]] = 1;
            }
        }
    }
}

void bfs(const GraphAdjList *G, int root, unsigned int *dist, int *path)
{
    assert(dist != NULL && path != NULL);
    memset(dist, INF, G->num_nodes * sizeof(int));
    // int *visit = (int *)malloc(G->num_nodes * sizeof(int));
    int *queue = (int *)calloc(G->num_nodes, sizeof(int));
    // assert(visit != NULL && queue != NULL);
    int queue_s = 0; // strat
    int queue_e = 0; // end
    // push root in queue
    queue[queue_e++] = root;
    dist[root] = 0;
    path[root] = root;
    // visit[root] = 1;
    // start BFS
    int this_node = 0;
    int dst_node = 0;
    int end = 0;
    unsigned int distance = 0;
    while (queue_e > queue_s)
    {
        distance++;
        end = queue_e;
        while (queue_s < end)
        {
            this_node = queue[queue_s++];
            for (int i = 0; i < G->degree; ++i)
            {
                dst_node = G->adj_list[this_node][i];
                if (dst_node != INF && dist[dst_node] == INF)
                {
                    queue[queue_e++] = dst_node;
                    dist[dst_node] = distance;
                    path[dst_node] = this_node;
                }
            }
        }
    }
    free(queue);
    queue = NULL;
}

void bfs_initial_path_dist(const GraphAdjList *G, unsigned int ***dist_p, int ***path_p)
{
    // initial dist & path
    unsigned int **dist;
    int **path;

    unsigned int *dist_array = (unsigned int *)calloc(G->num_nodes * G->num_nodes, sizeof(unsigned int));
    int *path_array = (int *)calloc(G->num_nodes * G->num_nodes, sizeof(int));
    assert(dist_array != NULL && path_array != NULL);

    dist = (unsigned int **)calloc(G->num_nodes, sizeof(unsigned int *));
    path = (int **)calloc(G->num_nodes, sizeof(int *));
    assert(dist != NULL && path != NULL);

    for (int i = 0; i < G->num_nodes; ++i)
    {
        dist[i] = dist_array + i * G->num_nodes;
        path[i] = path_array + i * G->num_nodes;
    }
    *dist_p = dist;
    *path_p = path;
}

void bfs_delete_path_dist(unsigned int **dist, int **path)
{
    free(dist[0]);
    free(dist);
    free(path[0]);
    free(path);
}

void bfs_shortest_path_algorithm(const GraphAdjList *G, unsigned int **dist, int **path)
{
    for (int i = 0; i < G->num_nodes; ++i)
        bfs(G, i, dist[i], path[i]);
}

void bfs_shortest_path_algorithm_parallel(const GraphAdjList *G, unsigned int **dist, int **path)
{
    // omp_set_num_threads(PROCS_NUM);
#pragma omp parallel for
    for (int i = 0; i < G->num_nodes; ++i)
        bfs(G, i, dist[i], path[i]);
}