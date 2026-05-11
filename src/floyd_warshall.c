#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <stdio.h>
#include "floyd_warshall.h"
#include "topology.h"
#include "omp.h"
#include "utils.h"

void floyd_algorithm(int num_nodes, unsigned int **dist, int **path)
{
    struct timeval start_time, end_time;
    // calculate
    gettimeofday(&start_time, NULL);
    int bypass = INF;
    for (int k = 0; k < num_nodes; ++k) // transit node
    {
        for (int i = 0; i < num_nodes; ++i) // source node
        {
            for (int j = 0; j < num_nodes; ++j) // destination node
            {
                bypass = dist[i][k] + dist[k][j];
                if (dist[i][j] > bypass)
                {
                    dist[i][j] = bypass;
                    path[i][j] = path[i][k];
                }
            }
        }
    }
    gettimeofday(&end_time, NULL);
    debug("##### time of \"shortest_path_floyd\" is : %ld\n", end_time.tv_sec - start_time.tv_sec);
}

void floyd_algorithm_step(int num_nodes, unsigned int **dist, int **path, int *node_list, int **matrix)
{
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    int bypass = INF;
    // int transit_node = 0;
    // len(node_list) == 4
    // clear
    for (int i = 0; i < num_nodes; ++i) // source node
    {
        for (int j = 0; j < num_nodes; ++j) // destination node
        {
            for (int k = 0; k < 4; ++k)
            {
                if (path[i][j] == node_list[k] || i == node_list[k])
                {
                    path[i][j] = j;
                    dist[i][j] = matrix[i][j];
                }
            }
        }
    }
    // print_matrix(dist, num_nodes, "clear dist");
    // print_matrix(path, num_nodes, "clear path");

    for (int k = 0; k < num_nodes; ++k) // transit node
    {
        for (int i = 0; i < num_nodes; ++i) // source node
        {
            for (int j = 0; j < num_nodes; ++j) // destination node
            {
                bypass = dist[i][k] + dist[k][j];
                if (dist[i][j] > bypass)
                {
                    dist[i][j] = bypass;
                    path[i][j] = path[i][k];
                }
            }
        }
    }

    // for (int k = 0; k < 4; ++k) // transit node
    // {
    //     transit_node = node_list[k];
    //     for (int i = 0; i < num_nodes; ++i) // source node
    //     {
    //         for (int j = 0; j < num_nodes; ++j) // destination node
    //         {
    //             bypass = dist[i][transit_node] + dist[transit_node][j];
    //             if (dist[i][j] > bypass)
    //             {
    //                 dist[i][j] = bypass;
    //                 path[i][j] = path[i][transit_node];
    //             }
    //         }
    //     }
    // }
    // for (int k = 0; k < num_nodes; ++k) // transit node
    // {
    //     for (int i = 0; i < 4; ++i) // source node
    //     {
    //         transit_node = node_list[i];
    //         for (int j = 0; j < num_nodes; ++j) // destination node
    //         {
    //             bypass = dist[transit_node][k] + dist[k][j];
    //             if (dist[transit_node][j] > bypass)
    //             {
    //                 dist[transit_node][j] = bypass;
    //                 path[transit_node][j] = path[transit_node][k];
    //             }
    //         }
    //     }
    // }
    // for (int k = 0; k < num_nodes; ++k) // transit node
    // {
    //     for (int i = 0; i < num_nodes; ++i) // source node
    //     {
    //         for (int j = 0; j < 4; ++j) // destination node
    //         {
    //             transit_node = node_list[j];
    //             bypass = dist[i][k] + dist[k][transit_node];
    //             if (dist[i][transit_node] > bypass)
    //             {
    //                 dist[i][transit_node] = bypass;
    //                 path[i][transit_node] = path[i][k];
    //             }
    //         }
    //     }
    // }
    gettimeofday(&end_time, NULL);
    printf("##### time of \"shortest_path_floyd_step\" is : %ld us\n", end_time.tv_usec - start_time.tv_usec);
}

void floyd_algorithm_parallel(int num_nodes, unsigned int **dist, int **path)
{
    // omp_set_num_threads(PROCS_NUM);
    struct timeval start_time, end_time;
    // calculate
    gettimeofday(&start_time, NULL);
    for (int k = 0; k < num_nodes; ++k) // transit node
    {
#pragma omp parallel for
        for (int i = 0; i < num_nodes; ++i) // source node
        {
            for (int j = 0; j < num_nodes; ++j) // destination node
            {
                if (dist[i][j] > dist[i][k] + dist[k][j])
                {
                    dist[i][j] = dist[i][k] + dist[k][j];
                    path[i][j] = path[i][k];
                }
            }
        }
    }
    gettimeofday(&end_time, NULL);
    debug("##### time of \"shortest_path_floyd_parallel\" is : %ld; %d CPU is used!\n", end_time.tv_sec - start_time.tv_sec, PROCS_NUM);
}

void floyd_check_path(const GraphAdjList *G, int **path)
{
    // check all paths are physical edge
    int *vld_dst = (int *)calloc(G->num_nodes, sizeof(int));
    assert(vld_dst != NULL);
    for (int i = 0; i < G->num_nodes; ++i) // source node
    {
        memset(vld_dst, -1, G->num_nodes * sizeof(int));
        vld_dst[i] = 1; // to self
        for (int j = 0; j < G->degree; ++j)
            if (G->adj_list[i][j] != INF)
                vld_dst[G->adj_list[i][j]] = 1;
        for (int j = 0; j < G->num_nodes; ++j) // destination node
            assert(vld_dst[path[i][j]] == 1);
    }
    // check cycle - live lock
    int pre_node;
    for (int i = 0; i < G->num_nodes; ++i)
    {
        for (int j = 0; j < G->num_nodes; ++j)
        {
            // printf("\n %d -> %d\n", i, j);
            memset(vld_dst, 0, G->num_nodes * sizeof(int));
            pre_node = path[i][j];
            while (pre_node != j)
            {
                // printf("%d -> ", pre_node);
                vld_dst[pre_node] = 1;
                pre_node = path[pre_node][j];
                assert(vld_dst[pre_node] == 0);
            }
            // printf("\n");
        }
    }
    free(vld_dst);
    vld_dst = NULL;
}

void floyd_check_dist(const GraphAdjList *G, unsigned int **dist)
{
    for (int i = 0; i < G->num_nodes; ++i)
    {
        for (int j = 0; j < G->num_nodes; ++j)
        {
            assert(dist[i][j] < INF);
        }
    }
}

void floyd_initial_path_dist(const GraphAdjList *G, unsigned int ***dist_p, int ***path_p)
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

    memset(dist_array, INF, G->num_nodes * G->num_nodes * sizeof(unsigned int));

    for (int i = 0; i < G->num_nodes; ++i)
    {
        dist[i] = dist_array + i * G->num_nodes;
        path[i] = path_array + i * G->num_nodes;
        for (int j = 0; j < G->num_nodes; ++j)
            path[i][j] = j;
        dist[i][i] = 0;
    }
    for (int i = 0; i < G->num_nodes; ++i)
    {
        for (int j = 0; j < G->degree; ++j)
        {
            if (G->adj_list[i][j] != INF)
                dist[i][G->adj_list[i][j]] = 1;
        }
    }
    *dist_p = dist;
    *path_p = path;
}

void floyd_delete_path_dist(unsigned int **dist, int **path)
{
    // free
    free(dist[0]);
    free(dist);
    free(path[0]);
    free(path);
}

double floyd_average_shortest_path_length(int num_nodes, unsigned int **dist)
{
    unsigned long total_length = 0;
    long double avg_length = 0;
    for (int i = 0; i < num_nodes; ++i)
    {
        for (int j = 0; j < num_nodes; ++j)
            total_length += dist[i][j];
    }
    avg_length = (long double)total_length / (long double)num_nodes / (long double)num_nodes;
    return (double)avg_length;
}

double floyd_shortest_path_variance(int num_nodes, unsigned int **dist)
{
    return variance(num_nodes * num_nodes, *dist);
}

unsigned int *floyd_initial_path_load_balance(const GraphAdjList *G, int **path)
{
    unsigned int *array = (unsigned int *)calloc(G->num_nodes * G->degree, sizeof(unsigned int));
    assert(array != NULL);
    int *vld_dst = (int *)calloc(G->num_nodes, sizeof(int));
    assert(vld_dst != NULL);
    for (int i = 0; i < G->num_nodes; ++i) // source node
    {
        memset(vld_dst, -1, G->num_nodes * sizeof(int));
        vld_dst[i] = 0; // to self
        for (int j = 0; j < G->degree; ++j)
            if (G->adj_list[i][j] != INF)
                vld_dst[G->adj_list[i][j]] = 0;
        for (int j = 0; j < G->num_nodes; ++j) // destination node
        {
            assert(vld_dst[path[i][j]] != -1);
            vld_dst[path[i][j]] += 1;
        }
        for (int j = 0; j < G->degree; ++j)
            array[i * G->degree + j] = (unsigned int)vld_dst[G->adj_list[i][j]];
    }
    free(vld_dst);
    vld_dst = NULL;
    return array;
}

double floyd_path_load_balance(int num_edges, unsigned int *array)
{
    return variance(num_edges, array);
}