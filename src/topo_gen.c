#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "topo_gen.h"
#include "cord.h"
#include "utils.h"
#include "floyd_warshall.h"
#include "dfs.h"
#include "sa_exchange.h"

// node_num = ary ** cube; degree = 2 * cube
GraphAdjList *torus_gen(int ary, int cube)
{
    GraphAdjList *G = (GraphAdjList *)calloc(1, sizeof(GraphAdjList));
    assert(G != NULL);
    G->num_nodes = 1;
    for (int i = 0; i < cube; ++i)
        G->num_nodes *= ary;
    G->degree = 2 * cube;
    // gen adjlist
    G->adj_list = (unsigned int **)calloc(G->num_nodes, sizeof(unsigned int *));
    assert(G->adj_list != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        G->adj_list[i] = (unsigned int *)calloc(G->degree, sizeof(unsigned int));
        assert(G->adj_list[i] != NULL);
    }

    int *cord = (int *)calloc(cube, sizeof(int));
    int *dst_cord = (int *)calloc(cube, sizeof(int));
    int *weight = (int *)calloc(cube, sizeof(int));
    assert(cord != NULL && weight != NULL && dst_cord != NULL);
    calculate_weight(ary, cube, weight);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        // calculate cord
        cord_fold(i, weight, cube, cord);
        // debug("%d : cord is : (%d, %d, %d)\n", i, cord[0], cord[1], cord[2]);
        memcpy(dst_cord, cord, cube * sizeof(int));
        for (int j = 0; j < cube; ++j)
        {
            dst_cord[j] += 1;
            if (dst_cord[j] >= ary)
                dst_cord[j] = 0;
            G->adj_list[i][2 * j] = cord_flatten(dst_cord, weight, cube);
            dst_cord[j] = cord[j]; // restore value
            dst_cord[j] -= 1;
            if (dst_cord[j] < 0)
                dst_cord[j] = ary - 1;
            G->adj_list[i][2 * j + 1] = cord_flatten(dst_cord, weight, cube);
            dst_cord[j] = cord[j]; // restore value
        }
    }
    free(cord);
    cord = NULL;
    free(dst_cord);
    dst_cord = NULL;
    free(weight);
    weight = NULL;
    return G;
}

GraphAdjList *swdc_torus_gen(int ary, int cube, int rand_degrees, unsigned int seed)
{
    GraphAdjList *G = (GraphAdjList *)calloc(1, sizeof(GraphAdjList));
    assert(G != NULL);
    G->num_nodes = 1;
    for (int i = 0; i < cube; ++i)
        G->num_nodes *= ary;
    G->degree = 2 * cube + rand_degrees;
    // gen adjlist
    G->adj_list = (unsigned int **)calloc(G->num_nodes, sizeof(unsigned int *));
    assert(G->adj_list != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        G->adj_list[i] = (unsigned int *)calloc(G->degree, sizeof(unsigned int));
        assert(G->adj_list[i] != NULL);
        memset(G->adj_list[i], INF, G->degree * sizeof(unsigned int));
    }
    // torus graph
    GraphAdjList *g = torus_gen(ary, cube);
    for (int i = 0; i < G->num_nodes; ++i)
        memcpy(G->adj_list[i], g->adj_list[i], 2 * cube * sizeof(int));
    delete_graph(g);
    // add random edges
    assert(rand_degrees < G->num_nodes);
    int random_ports = G->num_nodes * rand_degrees;
    int *set_i = (int *)calloc(random_ports, sizeof(int));
    int *set_j = (int *)calloc(random_ports, sizeof(int));
    assert(set_i != NULL && set_j != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        for (int j = 0; j < rand_degrees; ++j)
        {
            set_i[i * rand_degrees + j] = i;
            set_j[i * rand_degrees + j] = j;
        }
    }
    srand(seed);
    int src_idx, dst_idx, dst_pair;
    int swap_temp1, swap_temp2;
    for (int i = 0; i < 2 * random_ports; ++i) // shuffle
    {
        src_idx = rand() % random_ports;
        dst_idx = rand() % random_ports;
        while (src_idx == dst_idx)
            dst_idx = rand() % random_ports;
        swap_temp1 = set_i[src_idx];
        swap_temp2 = set_j[src_idx];
        set_i[src_idx] = set_i[dst_idx];
        set_j[src_idx] = set_j[dst_idx];
        set_i[dst_idx] = swap_temp1;
        set_j[dst_idx] = swap_temp2;
    }
    int conflict;
    for (int i = 0; i < random_ports - 1; i += 2)
    {
        conflict = 0;
        if (set_i[i + 1] == set_i[i])
            conflict = 1;
        for (int j = 0; j < G->degree; ++j)
        {
            conflict |= (set_i[i + 1] == G->adj_list[set_i[i]][j]) ? 1 : 0;
            conflict |= (set_i[i] == G->adj_list[set_i[i + 1]][j]) ? 1 : 0;
        }
        while (conflict == 1) // swap
        {
            conflict = 0;
            dst_pair = -1;
            if (i == (random_ports - 2))
            {
                assert(random_ports % 2 == 0);
                dst_idx = rand() % i;
                dst_pair = dst_idx ^ 1;
            }
            else if (i == (random_ports - 3))
            {
                assert(random_ports % 2 == 1);
                dst_idx = rand() % (i + 1);
                if (dst_idx == i)
                {
                    dst_idx = i + 2;
                    dst_pair = -1;
                }
            }
            else
            {
                dst_idx = (rand() % (random_ports - i - 2)) + i + 2;
            }
            if (set_i[dst_idx] == set_i[i])
                conflict = 1;
            for (int j = 0; j < G->degree; ++j)
            {
                conflict |= (set_i[dst_idx] == G->adj_list[set_i[i]][j]) ? 1 : 0;
                conflict |= (set_i[i] == G->adj_list[set_i[dst_idx]][j]) ? 1 : 0;
            }
            if (dst_pair != -1)
            {
                if (set_i[dst_pair] == set_i[i + 1])
                    conflict |= 1;
                for (int j = 0; j < G->degree; ++j)
                {
                    conflict |= (set_i[dst_pair] == G->adj_list[set_i[i + 1]][j]) ? 1 : 0;
                    conflict |= (set_i[i + 1] == G->adj_list[set_i[dst_pair]][j]) ? 1 : 0;
                }
            }
            if (conflict == 0)
            {
                swap_temp1 = set_i[i + 1];
                swap_temp2 = set_j[i + 1];
                set_i[i + 1] = set_i[dst_idx];
                set_j[i + 1] = set_j[dst_idx];
                set_i[dst_idx] = swap_temp1;
                set_j[dst_idx] = swap_temp2;
                if (dst_pair != -1)
                {
                    G->adj_list[set_i[dst_idx]][G->degree - rand_degrees + set_j[dst_idx]] = set_i[dst_pair];
                    G->adj_list[set_i[dst_pair]][G->degree - rand_degrees + set_j[dst_pair]] = set_i[dst_idx];
                }
            }
        }
        G->adj_list[set_i[i]][G->degree - rand_degrees + set_j[i]] = set_i[i + 1];
        G->adj_list[set_i[i + 1]][G->degree - rand_degrees + set_j[i + 1]] = set_i[i];
    }
    return G;
}

GraphAdjList *swdc_torus_local_gen(int ary, int cube, int rand_degrees, int dst_threshold, unsigned int seed)
{
    GraphAdjList *G = (GraphAdjList *)calloc(1, sizeof(GraphAdjList));
    assert(G != NULL);
    G->num_nodes = 1;
    for (int i = 0; i < cube; ++i)
        G->num_nodes *= ary;
    G->degree = 2 * cube + rand_degrees;
    // gen adjlist
    G->adj_list = (unsigned int **)calloc(G->num_nodes, sizeof(unsigned int *));
    assert(G->adj_list != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        G->adj_list[i] = (unsigned int *)calloc(G->degree, sizeof(unsigned int));
        assert(G->adj_list[i] != NULL);
        memset(G->adj_list[i], INF, G->degree * sizeof(unsigned int));
    }
    // torus graph
    GraphAdjList *g = torus_gen(ary, cube);
    for (int i = 0; i < G->num_nodes; ++i)
        memcpy(G->adj_list[i], g->adj_list[i], 2 * cube * sizeof(int));
    delete_graph(g);
    // add random edges (regular initial)
    if (rand_degrees % 2 == 0)
    {
        if (cube == 1) // Ring
        {
            assert(dst_threshold >= (1 + (rand_degrees / 2)));
            int dst_idx;
            for (int i = 0; i < G->num_nodes; ++i)
            {
                for (int j = 0; j < rand_degrees; ++j)
                {
                    dst_idx = (j % 2 == 0) ? (i + 2 + (j / 2)) : (i - 2 - (j / 2));
                    if (dst_idx >= G->num_nodes)
                        dst_idx -= G->num_nodes;
                    else if (dst_idx < 0)
                        dst_idx += G->num_nodes;
                    G->adj_list[i][j + 2 * cube] = dst_idx;
                }
            }
        }
        else if (cube == 2) // 2D Torus
        {
            assert(rand_degrees <= 4);
            assert(dst_threshold >= 2);
            int *cord = (int *)calloc(cube, sizeof(int));
            int *dst_cord = (int *)calloc(cube, sizeof(int));
            int *weight = (int *)calloc(cube, sizeof(int));
            assert(cord != NULL && weight != NULL && dst_cord != NULL);
            calculate_weight(ary, cube, weight);
            for (int i = 0; i < G->num_nodes; ++i)
            {
                // calculate cord
                cord_fold(i, weight, cube, cord);
                memcpy(dst_cord, cord, cube * sizeof(int));
                if (rand_degrees >= 2)
                {
                    dst_cord[0] += 1;
                    dst_cord[1] += 1;
                    if (dst_cord[0] >= ary)
                        dst_cord[0] = 0;
                    if (dst_cord[1] >= ary)
                        dst_cord[1] = 0;
                    G->adj_list[i][4] = cord_flatten(dst_cord, weight, cube);
                    memcpy(dst_cord, cord, cube * sizeof(int)); // restore value
                    dst_cord[0] -= 1;
                    dst_cord[1] -= 1;
                    if (dst_cord[0] < 0)
                        dst_cord[0] = ary - 1;
                    if (dst_cord[1] < 0)
                        dst_cord[1] = ary - 1;
                    G->adj_list[i][5] = cord_flatten(dst_cord, weight, cube);
                    memcpy(dst_cord, cord, cube * sizeof(int)); // restore value
                }
                if (rand_degrees >= 4)
                {
                    dst_cord[0] += 1;
                    dst_cord[1] -= 1;
                    if (dst_cord[0] >= ary)
                        dst_cord[0] = 0;
                    if (dst_cord[1] < 0)
                        dst_cord[1] = ary - 1;
                    G->adj_list[i][4] = cord_flatten(dst_cord, weight, cube);
                    memcpy(dst_cord, cord, cube * sizeof(int)); // restore value
                    dst_cord[0] -= 1;
                    dst_cord[1] += 1;
                    if (dst_cord[0] < 0)
                        dst_cord[0] = ary - 1;
                    if (dst_cord[1] >= ary)
                        dst_cord[1] = 0;
                    G->adj_list[i][5] = cord_flatten(dst_cord, weight, cube);
                    memcpy(dst_cord, cord, cube * sizeof(int)); // restore value
                }
            }
            free(cord);
            cord = NULL;
            free(dst_cord);
            dst_cord = NULL;
            free(weight);
            weight = NULL;
        }
        else
            assert(0);
    }
    else
        assert(0);
    // print_graph_adjlist(G);
    check_graph_adjlist(G);
    check_graph_dst_torus(G, ary, cube, dst_threshold);
    // shuffle
    srand(seed);
    int node[4];
    int port[4];
    int *visit = (int *)calloc(G->num_nodes, sizeof(int)); // connectivity
    int *stack = (int *)calloc(G->num_nodes, sizeof(int));
    assert(visit != NULL && stack != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        sa_swdc_exchange_local(G, ary, cube, rand_degrees, node, port, visit, stack, dst_threshold);
        // print_graph_adjlist(G);
        // check_graph_dst_torus(G, ary, cube, dst_threshold);
    }
    check_graph_adjlist(G);
    check_graph_dst_torus(G, ary, cube, dst_threshold);
    free(visit);
    free(stack);
    visit = NULL;
    stack = NULL;
    return G;
}

GraphAdjList *jellyfish_gen(int num_nodes, int degrees, unsigned int seed)
{
    GraphAdjList *G = (GraphAdjList *)calloc(1, sizeof(GraphAdjList));
    assert(G != NULL);
    G->num_nodes = num_nodes;
    G->degree = degrees;
    G->adj_list = (unsigned int **)calloc(G->num_nodes, sizeof(unsigned int *));
    assert(G->adj_list != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        G->adj_list[i] = (unsigned int *)calloc(G->degree, sizeof(unsigned int));
        assert(G->adj_list[i] != NULL);
        memset(G->adj_list[i], INF, G->degree * sizeof(unsigned int));
    }
    // add random edges
    assert(degrees < G->num_nodes);
    int random_ports = num_nodes * degrees;
    int *set_i = (int *)calloc(random_ports, sizeof(int));
    int *set_j = (int *)calloc(random_ports, sizeof(int));
    assert(set_i != NULL && set_j != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        for (int j = 0; j < degrees; ++j)
        {
            set_i[i * degrees + j] = i;
            set_j[i * degrees + j] = j;
        }
    }
    // shuffle
    srand(seed);
    int src_idx, dst_idx, dst_pair;
    int swap_temp1, swap_temp2;
    for (int i = 0; i < 2 * random_ports; ++i)
    {
        src_idx = rand() % random_ports;
        dst_idx = rand() % random_ports;
        while (src_idx == dst_idx)
            dst_idx = rand() % random_ports;
        swap_temp1 = set_i[src_idx];
        swap_temp2 = set_j[src_idx];
        set_i[src_idx] = set_i[dst_idx];
        set_j[src_idx] = set_j[dst_idx];
        set_i[dst_idx] = swap_temp1;
        set_j[dst_idx] = swap_temp2;
    }
    // construct
    int conflict;
    for (int i = 0; i < random_ports - 1; i += 2)
    {
        conflict = 0;
        if (set_i[i + 1] == set_i[i])
            conflict = 1;
        for (int j = 0; j < G->degree; ++j)
        {
            conflict |= (set_i[i + 1] == G->adj_list[set_i[i]][j]) ? 1 : 0;
            conflict |= (set_i[i] == G->adj_list[set_i[i + 1]][j]) ? 1 : 0;
        }
        while (conflict == 1) // swap
        {
            conflict = 0;
            dst_pair = -1;
            if (i == (random_ports - 2))
            {
                assert(random_ports % 2 == 0);
                dst_idx = rand() % i;
                dst_pair = dst_idx ^ 1;
            }
            else if (i == (random_ports - 3))
            {
                assert(random_ports % 2 == 1);
                dst_idx = rand() % (i + 1);
                if (dst_idx == i)
                {
                    dst_idx = i + 2;
                    dst_pair = -1;
                }
            }
            else
            {
                dst_idx = (rand() % (random_ports - i - 2)) + i + 2;
            }
            if (set_i[dst_idx] == set_i[i])
                conflict = 1;
            for (int j = 0; j < G->degree; ++j)
            {
                conflict |= (set_i[dst_idx] == G->adj_list[set_i[i]][j]) ? 1 : 0;
                conflict |= (set_i[i] == G->adj_list[set_i[dst_idx]][j]) ? 1 : 0;
            }
            if (dst_pair != -1)
            {
                if (set_i[dst_pair] == set_i[i + 1])
                    conflict |= 1;
                for (int j = 0; j < G->degree; ++j)
                {
                    conflict |= (set_i[dst_pair] == G->adj_list[set_i[i + 1]][j]) ? 1 : 0;
                    conflict |= (set_i[i + 1] == G->adj_list[set_i[dst_pair]][j]) ? 1 : 0;
                }
            }
            if (conflict == 0)
            {
                swap_temp1 = set_i[i + 1];
                swap_temp2 = set_j[i + 1];
                set_i[i + 1] = set_i[dst_idx];
                set_j[i + 1] = set_j[dst_idx];
                set_i[dst_idx] = swap_temp1;
                set_j[dst_idx] = swap_temp2;
                if (dst_pair != -1)
                {
                    G->adj_list[set_i[dst_idx]][G->degree - degrees + set_j[dst_idx]] = set_i[dst_pair];
                    G->adj_list[set_i[dst_pair]][G->degree - degrees + set_j[dst_pair]] = set_i[dst_idx];
                }
            }
        }
        G->adj_list[set_i[i]][G->degree - degrees + set_j[i]] = set_i[i + 1];
        G->adj_list[set_i[i + 1]][G->degree - degrees + set_j[i + 1]] = set_i[i];
    }
    // print_graph_adjlist(G);
    // check connectivity
    int *visit = (int *)calloc(G->num_nodes, sizeof(int));
    int *stack = (int *)calloc(G->num_nodes, sizeof(int));
    assert(visit != NULL && stack != NULL);
    int connectivity = -1;
    dfs(G, visit, stack);
    for (int i = 0; i < G->num_nodes; ++i) // find one disconnected node
        if (visit[i] == 0)
        {
            connectivity = i;
            break;
        }
    int d1, d1_p, d2, d2_p, s1, s1_p, s2, s2_p;
    while (connectivity != -1)
    {
        // build connection      //
        // select one connected node - D1
        d1 = rand() % G->num_nodes;
        while (visit[d1] == 0)
            d1 = rand() % G->num_nodes;
        d1_p = rand() % G->degree;
        while (G->adj_list[d1][d1_p] == INF)
            d1_p = rand() % G->degree;
        // get d2
        d2 = G->adj_list[d1][d1_p];
        for (int i = 0; i < G->degree; ++i)
        {
            if (G->adj_list[d2][i] == d1)
            {
                d2_p = i;
                break;
            }
        }
        // get s1
        s1 = connectivity;
        s1_p = rand() % G->degree;
        while (G->adj_list[s1][s1_p] == INF)
            s1_p = rand() % G->degree;
        // get s2
        s2 = G->adj_list[s1][s1_p];
        for (int i = 0; i < G->degree; ++i)
        {
            if (G->adj_list[s2][i] == s1)
            {
                s2_p = i;
                break;
            }
        }
        // connect s1 d1
        G->adj_list[s1][s1_p] = d1;
        G->adj_list[d1][d1_p] = s1;
        // connect s2 d2
        G->adj_list[s2][s2_p] = d2;
        G->adj_list[d2][d2_p] = s2;
        // print_graph_adjlist(G);
        // re-check connectivity  //
        connectivity = -1;
        dfs(G, visit, stack);
        for (int i = 0; i < G->num_nodes; ++i) // find one disconnected node
            if (visit[i] == 0)
            {
                connectivity = i;
                break;
            }
    }
    // print_graph_adjlist(G);
    return G;
}

GraphAdjList *jellyfish_local_gen(int num_nodes, int degrees, int dst_threshold, unsigned int seed)
{
    GraphAdjList *G = (GraphAdjList *)calloc(1, sizeof(GraphAdjList));
    assert(G != NULL);
    G->num_nodes = num_nodes;
    G->degree = degrees;
    G->adj_list = (unsigned int **)calloc(G->num_nodes, sizeof(unsigned int *));
    assert(G->adj_list != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        G->adj_list[i] = (unsigned int *)calloc(G->degree, sizeof(unsigned int));
        assert(G->adj_list[i] != NULL);
        memset(G->adj_list[i], INF, G->degree * sizeof(unsigned int));
    }
    // regular initial
    int dst_idx;
    if (degrees % 2 == 0)
    {
        for (int i = 0; i < G->num_nodes; ++i)
        {
            for (int j = 0; j < degrees; ++j)
            {
                dst_idx = (j % 2 == 0) ? (i + 1 + (j / 2)) : (i - 1 - (j / 2));
                if (dst_idx >= num_nodes)
                    dst_idx -= num_nodes;
                else if (dst_idx < 0)
                    dst_idx += num_nodes;
                G->adj_list[i][j] = dst_idx;
            }
        }
    }
    else
        assert(0);
    // print_graph_adjlist(G);
    // check_graph_adjlist(G);
    // check_graph_dst_ring(G, dst_threshold);
    // shuffle
    srand(seed);
    int node[4];
    int port[4];
    int *visit = (int *)calloc(G->num_nodes, sizeof(int)); // connectivity
    int *stack = (int *)calloc(G->num_nodes, sizeof(int));
    assert(visit != NULL && stack != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        sa_jellyfish_exchange_local(G, node, port, visit, stack, dst_threshold);
        // print_graph_adjlist(G);
        // check_graph_dst_ring(G, dst_threshold);
    }
    check_graph_adjlist(G);
    check_graph_dst_ring(G, dst_threshold);
    free(visit);
    free(stack);
    visit = NULL;
    stack = NULL;
    return G;
}

// node_num = ary ** 2; degree = 6
GraphAdjList *spinakker_gen(int ary)
{
    GraphAdjList *G = (GraphAdjList *)calloc(1, sizeof(GraphAdjList));
    assert(G != NULL);
    G->num_nodes = ary * ary;
    G->degree = 6;
    // gen adjlist
    G->adj_list = (unsigned int **)calloc(G->num_nodes, sizeof(unsigned int *));
    assert(G->adj_list != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        G->adj_list[i] = (unsigned int *)calloc(G->degree, sizeof(unsigned int));
        assert(G->adj_list[i] != NULL);
    }

    int *cord = (int *)calloc(2, sizeof(int));
    int *dst_cord = (int *)calloc(2, sizeof(int));
    int *weight = (int *)calloc(2, sizeof(int));
    assert(cord != NULL && weight != NULL && dst_cord != NULL);
    calculate_weight(ary, 2, weight);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        // calculate cord
        cord_fold(i, weight, 2, cord);
        // printf("%d : cord is : (%d, %d, %d)\n", i, cord[0], cord[1], cord[2]);
        // up
        memcpy(dst_cord, cord, 2 * sizeof(int));
        dst_cord[1] += 1;
        if (dst_cord[1] >= ary)
            dst_cord[1] = 0;
        G->adj_list[i][0] = cord_flatten(dst_cord, weight, 2);
        // down
        memcpy(dst_cord, cord, 2 * sizeof(int));
        dst_cord[1] -= 1;
        if (dst_cord[1] < 0)
            dst_cord[1] = ary - 1;
        G->adj_list[i][1] = cord_flatten(dst_cord, weight, 2);
        // right
        memcpy(dst_cord, cord, 2 * sizeof(int));
        dst_cord[0] += 1;
        if (dst_cord[0] >= ary)
            dst_cord[0] = 0;
        G->adj_list[i][2] = cord_flatten(dst_cord, weight, 2);
        // left
        memcpy(dst_cord, cord, 2 * sizeof(int));
        dst_cord[0] -= 1;
        if (dst_cord[0] < 0)
            dst_cord[0] = ary - 1;
        G->adj_list[i][3] = cord_flatten(dst_cord, weight, 2);
        // up right
        memcpy(dst_cord, cord, 2 * sizeof(int));
        dst_cord[0] += 1;
        dst_cord[1] += 1;
        if (dst_cord[0] >= ary)
            dst_cord[0] = 0;
        if (dst_cord[1] >= ary)
            dst_cord[1] = 0;
        G->adj_list[i][4] = cord_flatten(dst_cord, weight, 2);
        // down left
        memcpy(dst_cord, cord, 2 * sizeof(int));
        dst_cord[0] -= 1;
        dst_cord[1] -= 1;
        if (dst_cord[0] < 0)
            dst_cord[0] = ary - 1;
        if (dst_cord[1] < 0)
            dst_cord[1] = ary - 1;
        G->adj_list[i][5] = cord_flatten(dst_cord, weight, 2);
    }
    free(cord);
    cord = NULL;
    free(dst_cord);
    dst_cord = NULL;
    free(weight);
    weight = NULL;
    return G;
}

// node_num = ary * cube; degree = 2 * cube
GraphAdjList *camcube_gen(int ary);

// host num = k^3 / 4; switch num = 5k^2 / 4; link num = k^3 / 2
GraphAdjList *fat_tree_gen(int ary);