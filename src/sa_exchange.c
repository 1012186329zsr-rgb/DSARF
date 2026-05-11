#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "sa_exchange.h"
#include "floyd_warshall.h"
#include "topo_gen.h"
#include "dfs.h"
#include "utils.h"
#include "dfs.h"
#include "cord.h"

void sa_swdc_exchange(GraphAdjList *G, int ary, int cube, int rand_degrees, int *node, int *port, int *visit, int *stack, int dst_threshold)
{
    assert(dst_threshold == G->num_nodes);
    assert(ary > 0 && cube > 0);
    int connectivity = 1;
    int cond = 1;
    int fix_degree = G->degree - rand_degrees;
    while (connectivity == 1)
    {
        // s1
        node[0] = rand() % G->num_nodes;
        port[0] = (rand() % rand_degrees) + fix_degree;
        while (G->adj_list[node[0]][port[0]] == INF)
            port[0] = (rand() % rand_degrees) + fix_degree;
        // s2
        node[1] = G->adj_list[node[0]][port[0]];
        for (int i = 0; i < G->degree; ++i)
        {
            if (G->adj_list[node[1]][i] == node[0])
            {
                port[1] = i;
                break;
            }
        }
        // d1
        cond = 1;
        while (cond)
        {
            cond = 0;
            node[2] = rand() % G->num_nodes;
            cond |= ((node[2] == node[0]) || (node[2] == node[1])); // d1 != s1 && d1 != s2
            for (int i = 0; i < G->degree; ++i)                     // d1 should not connect to s1
                cond |= (G->adj_list[node[2]][i] == node[0]);
            if (cond == 0)
            {
                port[2] = (rand() % rand_degrees) + fix_degree;
                while (G->adj_list[node[2]][port[2]] == INF)
                    port[2] = (rand() % rand_degrees) + fix_degree;
                // d2
                node[3] = G->adj_list[node[2]][port[2]];
                cond |= ((node[3] == node[0]) || (node[3] == node[1])); // d2 != s1 && d2 != s2
                for (int i = 0; i < G->degree; ++i)                     // d2 shoule not connect to s2
                    cond |= (G->adj_list[node[3]][i] == node[1]);
            }
        }
        // d2 port
        for (int i = 0; i < G->degree; ++i)
        {
            if (G->adj_list[node[3]][i] == node[2])
            {
                port[3] = i;
                break;
            }
        }
        // connect s1 d1
        G->adj_list[node[0]][port[0]] = node[2];
        G->adj_list[node[2]][port[2]] = node[0];
        // connect s2 d2
        G->adj_list[node[1]][port[1]] = node[3];
        G->adj_list[node[3]][port[3]] = node[1];
        // check connectivity
        connectivity = 0;
        dfs(G, visit, stack);
        for (int i = 0; i < G->num_nodes; ++i) // find one disconnected node
            if (visit[i] == 0)
            {
                connectivity = 1;
                break;
            }
        if (connectivity == 1) // recover
        {
            G->adj_list[node[0]][port[0]] = node[1];
            G->adj_list[node[1]][port[1]] = node[0];
            G->adj_list[node[2]][port[2]] = node[3];
            G->adj_list[node[3]][port[3]] = node[2];
        }
    }
}

void sa_swdc_exchange_local(GraphAdjList *G, int ary, int cube, int rand_degrees, int *node, int *port, int *visit, int *stack, int dst_threshold)
{
    int connectivity = 1;
    int cond = 1;
    int fix_degree = G->degree - rand_degrees;
    int *port_vld = calloc(G->degree, sizeof(int));
    assert(port_vld != NULL);
    int *cord1 = (int *)calloc(cube, sizeof(int));
    int *cord2 = (int *)calloc(cube, sizeof(int));
    int *weight = (int *)calloc(cube, sizeof(int));
    assert(cord1 != NULL && weight != NULL && cord2 != NULL);
    calculate_weight(ary, cube, weight);
    int *delta = (int *)calloc(cube, sizeof(int));
    int delta_rest;
    while (connectivity == 1)
    {
        // s1
        node[0] = rand() % G->num_nodes;
        port[0] = (rand() % rand_degrees) + fix_degree;
        while (G->adj_list[node[0]][port[0]] == INF)
            port[0] = (rand() % rand_degrees) + fix_degree;
        // s2
        node[1] = G->adj_list[node[0]][port[0]];
        for (int i = fix_degree; i < G->degree; ++i)
        {
            if (G->adj_list[node[1]][i] == node[0])
            {
                port[1] = i;
                break;
            }
        }
        // d1
        cond = 1;
        while (cond)
        {
            cond = 0;
            delta_rest = dst_threshold + 1;
            for (int i = 0; i < cube; ++i)
            {
                delta[i] = rand() % delta_rest;
                delta_rest -= delta[i];
            }
            cord_fold(node[0], weight, cube, cord1);
            for (int i = 0; i < cube; ++i)
            {
                if (rand() % 2 == 0)
                    cord1[i] += delta[i];
                else
                    cord1[i] -= delta[i];
                if (cord1[i] < 0)
                    cord1[i] = ary - 1;
                else if (cord1[i] >= ary)
                    cord1[i] = 0;
            }
            node[2] = cord_flatten(cord1, weight, cube);            // distance between s1 & d1 should less than dst_threshold
            cond |= ((node[2] == node[0]) || (node[2] == node[1])); // d1 != s1 && d1 != s2
            for (int i = 0; i < G->degree; ++i)                     // d1 should not connect to s1
                cond |= (G->adj_list[node[2]][i] == node[0]);
            if (cond == 0)
            {
                memset(port_vld, 0, G->degree * sizeof(int));
                cond = 1;
                for (int i = fix_degree; i < G->degree; ++i)
                {
                    cord_fold(node[1], weight, cube, cord1);
                    cord_fold(G->adj_list[node[2]][i], weight, cube, cord2);
                    if (dst_torus(cord1, cord2, ary, cube) <= dst_threshold) // distance between d2 & s2 should less than dst_threshold
                    {
                        port_vld[i] = 1;
                        cond = 0;
                    }
                }
                if (cond == 1)
                    continue;
                port[2] = (rand() % rand_degrees) + fix_degree;
                while (G->adj_list[node[2]][port[2]] == INF || port_vld[port[2]] == 0)
                    port[2] = (rand() % rand_degrees) + fix_degree;
                // d2
                node[3] = G->adj_list[node[2]][port[2]];
                cond |= ((node[3] == node[0]) || (node[3] == node[1])); // d2 != s1 && d2 != s2
                for (int i = 0; i < G->degree; ++i)                     // d2 shoule not connect to s2
                    cond |= (G->adj_list[node[3]][i] == node[1]);
            }
        }
        // d2 port
        for (int i = fix_degree; i < G->degree; ++i)
        {
            if (G->adj_list[node[3]][i] == node[2])
            {
                port[3] = i;
                break;
            }
        }
        // connect s1 d1
        G->adj_list[node[0]][port[0]] = node[2];
        G->adj_list[node[2]][port[2]] = node[0];
        // connect s2 d2
        G->adj_list[node[1]][port[1]] = node[3];
        G->adj_list[node[3]][port[3]] = node[1];
        // check connectivity
        connectivity = 0;
        dfs(G, visit, stack);
        for (int i = 0; i < G->num_nodes; ++i) // find one disconnected node
            if (visit[i] == 0)
            {
                connectivity = 1;
                break;
            }
        if (connectivity == 1) // recover
        {
            G->adj_list[node[0]][port[0]] = node[1];
            G->adj_list[node[1]][port[1]] = node[0];
            G->adj_list[node[2]][port[2]] = node[3];
            G->adj_list[node[3]][port[3]] = node[2];
        }
    }
    // print_graph_adjlist(G);
    // check_graph_adjlist(G);
    // check_graph_dst_torus(G, ary, cube, dst_threshold);
    free(port_vld);
    port_vld = NULL;
    free(cord1);
    cord1 = NULL;
    free(cord2);
    cord2 = NULL;
    free(weight);
    weight = NULL;
    free(delta);
    delta = NULL;
}

void sa_jellyfish_exchange(GraphAdjList *G, int *node, int *port, int *visit, int *stack, int dst_threshold)
{
    assert(dst_threshold == G->num_nodes);
    int connectivity = 1;
    int cond = 1;
    while (connectivity == 1)
    {
        // s1
        node[0] = rand() % G->num_nodes;
        port[0] = rand() % G->degree;
        while (G->adj_list[node[0]][port[0]] == INF)
            port[0] = rand() % G->degree;
        // s2
        node[1] = G->adj_list[node[0]][port[0]];
        for (int i = 0; i < G->degree; ++i)
        {
            if (G->adj_list[node[1]][i] == node[0])
            {
                port[1] = i;
                break;
            }
        }
        // d1
        cond = 1;
        while (cond)
        {
            cond = 0;
            node[2] = rand() % G->num_nodes;
            cond |= ((node[2] == node[0]) || (node[2] == node[1])); // d1 != s1 && d1 != s2
            for (int i = 0; i < G->degree; ++i)                     // d1 should not connect to s1
                cond |= (G->adj_list[node[2]][i] == node[0]);
            if (cond == 0)
            {
                port[2] = rand() % G->degree;
                while (G->adj_list[node[2]][port[2]] == INF)
                    port[2] = rand() % G->degree;
                // d2
                node[3] = G->adj_list[node[2]][port[2]];
                cond |= ((node[3] == node[0]) || (node[3] == node[1])); // d2 != s1 && d2 != s2
                for (int i = 0; i < G->degree; ++i)                     // d2 shoule not connect to s2
                    cond |= (G->adj_list[node[3]][i] == node[1]);
            }
        }
        // d2 port
        for (int i = 0; i < G->degree; ++i)
        {
            if (G->adj_list[node[3]][i] == node[2])
            {
                port[3] = i;
                break;
            }
        }
        // connect s1 d1
        G->adj_list[node[0]][port[0]] = node[2];
        G->adj_list[node[2]][port[2]] = node[0];
        // connect s2 d2
        G->adj_list[node[1]][port[1]] = node[3];
        G->adj_list[node[3]][port[3]] = node[1];
        // check connectivity
        connectivity = 0;
        dfs(G, visit, stack);
        for (int i = 0; i < G->num_nodes; ++i) // find one disconnected node
            if (visit[i] == 0)
            {
                connectivity = 1;
                break;
            }
        if (connectivity == 1) // recover
        {
            G->adj_list[node[0]][port[0]] = node[1];
            G->adj_list[node[1]][port[1]] = node[0];
            G->adj_list[node[2]][port[2]] = node[3];
            G->adj_list[node[3]][port[3]] = node[2];
        }
    }
}

void sa_jellyfish_exchange_local(GraphAdjList *G, int *node, int *port, int *visit, int *stack, int dst_threshold)
{
    int connectivity = 1;
    int cond = 1;
    int *port_vld = calloc(G->degree, sizeof(int));
    assert(port_vld != NULL);
    int dst;
    int half = G->num_nodes / 2;
    while (connectivity == 1)
    {
        // s1
        node[0] = rand() % G->num_nodes;
        port[0] = rand() % G->degree;
        while (G->adj_list[node[0]][port[0]] == INF)
            port[0] = rand() % G->degree;
        // s2
        node[1] = G->adj_list[node[0]][port[0]];
        for (int i = 0; i < G->degree; ++i)
        {
            if (G->adj_list[node[1]][i] == node[0])
            {
                port[1] = i;
                break;
            }
        }
        // d1
        cond = 1;
        while (cond)
        {
            cond = 0;
            node[2] = node[0] - dst_threshold + (rand() % (2 * dst_threshold + 1)); // distance between s1 & d1 should less than dst_threshold
            if (node[2] >= G->num_nodes)
                node[2] -= G->num_nodes;
            else if (node[2] < 0)
                node[2] += G->num_nodes;
            cond |= ((node[2] == node[0]) || (node[2] == node[1])); // d1 != s1 && d1 != s2
            for (int i = 0; i < G->degree; ++i)                     // d1 should not connect to s1
                cond |= (G->adj_list[node[2]][i] == node[0]);
            if (cond == 0)
            {
                memset(port_vld, 0, G->degree * sizeof(int));
                cond = 1;
                for (int i = 0; i < G->degree; ++i)
                {
                    dst = abs(G->adj_list[node[2]][i] - node[1]); // distance between d2 & s2 should less than dst_threshold
                    dst = (dst > half) ? G->num_nodes - dst : dst;
                    if (dst <= dst_threshold)
                    {
                        port_vld[i] = 1;
                        cond = 0;
                    }
                }
                if (cond == 1)
                    continue;
                port[2] = rand() % G->degree;
                while (G->adj_list[node[2]][port[2]] == INF || port_vld[port[2]] == 0)
                    port[2] = rand() % G->degree;
                // d2
                node[3] = G->adj_list[node[2]][port[2]];
                cond |= ((node[3] == node[0]) || (node[3] == node[1])); // d2 != s1 && d2 != s2
                for (int i = 0; i < G->degree; ++i)                     // d2 shoule not connect to s2
                    cond |= (G->adj_list[node[3]][i] == node[1]);
            }
        }
        // d2 port
        for (int i = 0; i < G->degree; ++i)
        {
            if (G->adj_list[node[3]][i] == node[2])
            {
                port[3] = i;
                break;
            }
        }
        // connect s1 d1
        G->adj_list[node[0]][port[0]] = node[2];
        G->adj_list[node[2]][port[2]] = node[0];
        // connect s2 d2
        G->adj_list[node[1]][port[1]] = node[3];
        G->adj_list[node[3]][port[3]] = node[1];
        // check connectivity
        connectivity = 0;
        dfs(G, visit, stack);
        for (int i = 0; i < G->num_nodes; ++i) // find one disconnected node
            if (visit[i] == 0)
            {
                connectivity = 1;
                break;
            }
        if (connectivity == 1) // recover
        {
            G->adj_list[node[0]][port[0]] = node[1];
            G->adj_list[node[1]][port[1]] = node[0];
            G->adj_list[node[2]][port[2]] = node[3];
            G->adj_list[node[3]][port[3]] = node[2];
        }
    }
    free(port_vld);
    port_vld = NULL;
}