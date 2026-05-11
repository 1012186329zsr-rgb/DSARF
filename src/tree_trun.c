#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "tree_turn.h"
#include "floyd_warshall.h"
#include "omp.h"

TreeCoordinate *gen_tree_coordinate(const GraphAdjList *G, int root, int type)
{
    /**** initial ****/
    TreeCoordinate *tc = (TreeCoordinate *)calloc(1, sizeof(TreeCoordinate));
    assert(tc != NULL);
    tc->x = (int *)calloc(G->num_nodes, sizeof(int));
    tc->y = (int *)calloc(G->num_nodes, sizeof(int));
    assert(tc->x != NULL && tc->y != NULL);
    memset(tc->y, INF, G->num_nodes * sizeof(int));
    /**** build BFS Tree ****/
    assert(type == 1);
    int **tree_matrix = (int **)calloc(G->num_nodes, sizeof(int *));
    assert(tree_matrix != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        tree_matrix[i] = (int *)calloc(G->num_nodes, sizeof(int));
        assert(tree_matrix[i] != NULL);
    }
    int *queue = (int *)calloc(G->num_nodes, sizeof(int));
    assert(queue != NULL);
    int queue_s = 0; // strat
    int queue_e = 0; // end
    // push root in queue
    queue[queue_e++] = root;
    tc->y[root] = 0;
    // start BFS
    int this_node = 0;
    int dst_node = 0;
    int end = 0;
    int distance = 0;
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
                if (dst_node != INF && tc->y[dst_node] == INF)
                {
                    queue[queue_e++] = dst_node;
                    tc->y[dst_node] = distance;
                    tree_matrix[this_node][dst_node] = 1;
                }
            }
        }
    }
    /**** preorder traversal ****/
    int stack_ptr = 0;
    queue[stack_ptr++] = root;
    distance = 0;
    while (stack_ptr > 0)
    {
        this_node = queue[--stack_ptr];
        tc->x[this_node] = distance;
        for (int i = 0; i < G->num_nodes; ++i)
            if (tree_matrix[this_node][i] == 1)
                queue[stack_ptr++] = i;
        distance++;
    }
    free(queue);
    for (int i = 0; i < G->num_nodes; ++i)
        free(tree_matrix[i]);
    free(tree_matrix);
    return tc;
}

void delete_tc(TreeCoordinate *tc)
{
    free(tc->x);
    free(tc->y);
    free(tc);
}

int is_lu(int x1, int y1, int x2, int y2)
{
    return (x2 < x1) && (y2 < y1);
}

int is_l(int x1, int y1, int x2, int y2)
{
    return (x2 < x1) && (y2 == y1);
}

int is_ld(int x1, int y1, int x2, int y2)
{
    return (x2 < x1) && (y2 > y1);
}

int is_ru(int x1, int y1, int x2, int y2)
{
    return (x2 > x1) && (y2 < y1);
}

int is_r(int x1, int y1, int x2, int y2)
{
    return (x2 > x1) && (y2 == y1);
}

int is_rd(int x1, int y1, int x2, int y2)
{
    return (x2 > x1) && (y2 > y1);
}

int check_tree_trun(int x1, int y1, int x2, int y2, int x3, int y3)
{
    if ((is_l(x1, y1, x2, y2) && is_lu(x2, y2, x3, y3)) ||
        (is_ld(x1, y1, x2, y2) && is_lu(x2, y2, x3, y3)) ||
        (is_ru(x1, y1, x2, y2) && is_lu(x2, y2, x3, y3)) ||
        (is_r(x1, y1, x2, y2) && is_lu(x2, y2, x3, y3)) ||
        (is_rd(x1, y1, x2, y2) && is_lu(x2, y2, x3, y3)) ||
        (is_ru(x1, y1, x2, y2) && is_l(x2, y2, x3, y3)) ||
        (is_r(x1, y1, x2, y2) && is_l(x2, y2, x3, y3)) ||
        (is_ru(x1, y1, x2, y2) && is_ld(x2, y2, x3, y3)) ||
        (is_ru(x1, y1, x2, y2) && is_r(x2, y2, x3, y3)) ||
        (is_ru(x1, y1, x2, y2) && is_rd(x2, y2, x3, y3)))
        return 0;
    else
        return 1;
}

void tree_trun_bfs(const GraphAdjList *G, const TreeCoordinate *tc, int root, unsigned int *dist, int *path)
{
    assert(dist != NULL && path != NULL && tc != NULL);
    memset(dist, INF, G->num_nodes * sizeof(int));
    int *queue = (int *)calloc(G->num_nodes, sizeof(int));
    int queue_s = 0; // strat
    int queue_e = 0; // end
    // push root in queue
    queue[queue_e++] = root;
    dist[root] = 0;
    path[root] = root;
    // start BFS
    int this_node = 0;
    int pre_node = 0;
    int dst_node = 0;
    int end = 0;
    while (queue_e > queue_s)
    {
        end = queue_e;
        while (queue_s < end)
        {
            this_node = queue[queue_s++];
            pre_node = path[this_node];
            for (int i = 0; i < G->degree; ++i)
            {
                dst_node = G->adj_list[this_node][i];
                if (dst_node != INF && dist[dst_node] == INF && (pre_node == this_node || check_tree_trun(tc->x[pre_node], tc->y[pre_node], tc->x[this_node], tc->y[this_node], tc->x[dst_node], tc->y[dst_node]) != 0))
                {
                    queue[queue_e++] = dst_node;
                    dist[dst_node] = dist[this_node] + 1;
                    path[dst_node] = this_node;
                }
            }
        }
    }
    free(queue);
    queue = NULL;
}

void tree_turn_bfs_shortest_path_algorithm_parallel(const GraphAdjList *G, const TreeCoordinate *tc, unsigned int **dist, int **path)
{
    // omp_set_num_threads(PROCS_NUM);
#pragma omp parallel for
    for (int i = 0; i < G->num_nodes; ++i)
        tree_trun_bfs(G, tc, i, dist[i], path[i]);
    floyd_check_dist(G, dist);
}

void tree_turn_floyd_algorithm_parallel(int num_nodes, const TreeCoordinate *tc, unsigned int **dist, int **path)
{
    // omp_set_num_threads(PROCS_NUM);
    // calculate
    int x1, y1, x2, y2, x3, y3;
    int pre_node, post_node;
    for (int k = 0; k < num_nodes; ++k) // transit node
    {
        x2 = tc->x[k];
        y2 = tc->y[k];
        // #pragma omp parallel for
        for (int i = 0; i < num_nodes; ++i) // source node
        {
            for (int j = 0; j < num_nodes; ++j) // destination node
            {
                if (dist[i][j] > dist[i][k] + dist[k][j])
                {
                    // pre_node
                    if (path[i][k] == k)
                        pre_node = i;
                    else
                    {
                        pre_node = path[i][k];
                        while (path[pre_node][k] != k)
                            pre_node = path[pre_node][k];
                    }
                    // assert((i != k && pre_node != k) || (i == k && pre_node == k));
                    x1 = tc->x[pre_node];
                    y1 = tc->y[pre_node];
                    // post_node
                    post_node = path[k][j];
                    // assert((j != k && post_node != j) || (j == k && post_node == j));
                    x3 = tc->x[post_node];
                    y3 = tc->y[post_node];
                    if (check_tree_trun(x1, y1, x2, y2, x3, y3) != 0)
                    {
                        dist[i][j] = dist[i][k] + dist[k][j];
                        path[i][j] = path[i][k];
                    }
                }
            }
        }
    }
}

void tree_turn_check_path(int num_nodes, int **path, TreeCoordinate *tc)
{
    // turn model check
    int pre_node, this_node, post_node;
    for (int i = 0; i < num_nodes; ++i)
    {
        for (int j = 0; j < num_nodes; ++j)
        {
            if (i == j)
                continue;
            pre_node = i;
            this_node = path[i][j];
            if (this_node == j)
                continue;
            post_node = path[this_node][j];
            while (1)
            {
                assert(check_tree_trun(tc->x[pre_node], tc->y[pre_node], tc->x[this_node], tc->y[this_node], tc->x[post_node], tc->y[post_node]) != 0);
                pre_node = this_node;
                this_node = post_node;
                if (this_node == j)
                    break;
                post_node = path[this_node][j];
            }
        }
    }
}

int *statistic_path(const GraphAdjList *G, const TreeCoordinate *tc)
{
    int *array = (int *)calloc(6, sizeof(int));
    assert(array != NULL);
    int x1, y1, x2, y2;
    for (int i = 0; i < G->num_nodes; ++i)
    {
        x1 = tc->x[i];
        y1 = tc->y[i];
        for (int j = 0; j < G->degree; ++j)
        {
            if (G->adj_list[i][j] != INF)
            {
                x2 = tc->x[G->adj_list[i][j]];
                y2 = tc->y[G->adj_list[i][j]];
                if (is_lu(x1, y1, x2, y2))
                    array[0] += 1;
                else if (is_l(x1, y1, x2, y2))
                    array[1] += 1;
                else if (is_ld(x1, y1, x2, y2))
                    array[2] += 1;
                else if (is_ru(x1, y1, x2, y2))
                    array[3] += 1;
                else if (is_r(x1, y1, x2, y2))
                    array[4] += 1;
                else if (is_rd(x1, y1, x2, y2))
                    array[5] += 1;
                else
                    assert(0);
            }
        }
    }
    return array;
}