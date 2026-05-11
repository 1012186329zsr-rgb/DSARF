#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "sa.h"
#include "floyd_warshall.h"
#include "topo_gen.h"
#include "dfs.h"
#include "utils.h"
#include "dfs.h"
#include "sa_exchange.h"

GraphAdjList *sa_swdc_torus_gen(int ary, int cube, int rand_degrees, int dst_threshold, unsigned int seed)
{
    printf("\n------------------------ Start SA of SWDC ------------------------\n");
    // generate Graph
    GraphAdjList *G = swdc_torus_gen(ary, cube, rand_degrees, seed);
    if (dst_threshold < G->num_nodes)
    {
        delete_graph(G);
        G = swdc_torus_local_gen(ary, cube, rand_degrees, dst_threshold, seed);
    }
    check_graph_adjlist(G);
    // parameter of simulated annealing
    double t_current = SA_T_INITIAL;
    int iter = 0;
    // print parameters
    printf("\n******** Parameter Setting ********\n");
    printf("\tary = %d\n", ary);
    printf("\tcube = %d\n", cube);
    printf("\trand_degrees = %d\n", rand_degrees);
    printf("\tdst_threshold = %d\n", dst_threshold);
    printf("\tseed = %u\n\n", seed);
    printf("t_current = %lf\n", t_current);
    printf("t_stop = %lf\n", SA_T_STOP);
    printf("alpha = %lf\n", SA_ALPHA);
    printf("sa_not_katen_max = %d\n", SA_NOT_TAKEN_MAX);
    // some variables
    double cost;
    double cost_new;
    double delta_cost;
    double accept_rate;
    double avg_shortest_path = 0; // initial value
    double shortest_path_var = 0;
    double path_load_balance = 0;
    double avg, var, balance; // step value
    int taken;
    unsigned int **dist = NULL;
    int **path = NULL;
    int node[4], port[4];                                  // s1 s2 d1 d2
    int *visit = (int *)calloc(G->num_nodes, sizeof(int)); // connectivity
    int *stack = (int *)calloc(G->num_nodes, sizeof(int));
    assert(visit != NULL && stack != NULL);
    int not_taken_cnt = 0;
    printf("\n******** Start Searching ********\n");
    // print_graph_adjlist(G);
    sa_cost_bfs(G, &dist, &path, &avg_shortest_path, &shortest_path_var, &path_load_balance);
    cost = 3;
    printf("--- initial avg %lf, var %lf, balance %lf\n", avg_shortest_path, shortest_path_var, path_load_balance);
    void (*func_exchange)(GraphAdjList *, int, int, int, int *, int *, int *, int *, int);
    if (dst_threshold == G->num_nodes)
        func_exchange = sa_swdc_exchange;
    else
        func_exchange = sa_swdc_exchange_local;
    while ((not_taken_cnt < SA_NOT_TAKEN_MAX) && (t_current > SA_T_STOP))
    {
        t_current *= SA_ALPHA;
        // random select and exchange
        func_exchange(G, ary, cube, rand_degrees, node, port, visit, stack, dst_threshold);
        // calculate cost
        sa_cost_bfs_step(G, &dist, &path, &avg, &var, &balance);
        cost_new = avg / avg_shortest_path + var / shortest_path_var + balance / path_load_balance;
        delta_cost = cost - cost_new;
        accept_rate = exp(delta_cost * 30000 / t_current) * 100;
        taken = ((rand() % 100) < accept_rate) ? 1 : 0;
        printf("-------- iter %d; t_current = %lf; delta_cost = %lf; cost = %lf, taken = %d; avg = %lf, var = %lf, balance = %lf, P = %lf\n", iter++, t_current, delta_cost, cost_new, taken, avg, var, balance, accept_rate);
        if (taken == 0) // recover
        {
            G->adj_list[node[0]][port[0]] = node[1];
            G->adj_list[node[1]][port[1]] = node[0];
            G->adj_list[node[2]][port[2]] = node[3];
            G->adj_list[node[3]][port[3]] = node[2];
            not_taken_cnt++;
        }
        else
        {
            not_taken_cnt = 0;
            cost = cost_new;
        }
    }
    printf("\n******** End Searching ********\n");
    check_graph_dst_torus(G, ary, cube, dst_threshold);
    check_graph_adjlist(G);
    bfs_delete_path_dist(dist, path);
    free(visit);
    free(stack);
    visit = NULL;
    stack = NULL;
    return G;
}

GraphAdjList *sa_jellyfish_gen(int num_nodes, int degrees, int dst_threshold, unsigned int seed)
{
    printf("\n------------------------ Start SA of Jellyfish ------------------------\n");
    // generate Graph
    GraphAdjList *G = NULL;
    if (dst_threshold == num_nodes)
        G = jellyfish_gen(num_nodes, degrees, seed);
    else
        G = jellyfish_local_gen(num_nodes, degrees, dst_threshold, seed);
    check_graph_adjlist(G);
    // parameter of simulated annealing
    double t_current = SA_T_INITIAL;
    int iter = 0;
    // print parameters
    printf("\n******** Parameter Setting ********\n");
    printf("\tnum_nodes = %d\n", num_nodes);
    printf("\tdegrees = %d\n", degrees);
    printf("\tdst_threshold = %d\n", dst_threshold);
    printf("\tseed = %u\n\n", seed);
    printf("t_current = %lf\n", t_current);
    printf("t_stop = %lf\n", SA_T_STOP);
    printf("alpha = %lf\n", SA_ALPHA);
    printf("sa_not_katen_max = %d\n", SA_NOT_TAKEN_MAX);
    // some variables
    double cost;
    double cost_new;
    double delta_cost;
    double accept_rate;
    double avg_shortest_path = 0; // initial value
    double shortest_path_var = 0;
    double path_load_balance = 0;
    double avg, var, balance; // step value
    int taken;
    unsigned int **dist = NULL;
    int **path = NULL;
    int node[4], port[4];                                  // s1 s2 d1 d2
    int *visit = (int *)calloc(G->num_nodes, sizeof(int)); // connectivity
    int *stack = (int *)calloc(G->num_nodes, sizeof(int));
    assert(visit != NULL && stack != NULL);
    int not_taken_cnt = 0;
    printf("\n******** Start Searching ********\n");
    sa_cost_bfs(G, &dist, &path, &avg_shortest_path, &shortest_path_var, &path_load_balance);
    cost = 3;
    printf("--- initial avg %lf, var %lf, balance %lf\n", avg_shortest_path, shortest_path_var, path_load_balance);
    void (*func_exchange)(GraphAdjList *, int *, int *, int *, int *, int);
    if (dst_threshold == num_nodes)
        func_exchange = sa_jellyfish_exchange;
    else
        func_exchange = sa_jellyfish_exchange_local;
    while ((not_taken_cnt < SA_NOT_TAKEN_MAX) && (t_current > SA_T_STOP))
    {
        t_current *= SA_ALPHA;
        // random select and exchange
        func_exchange(G, node, port, visit, stack, dst_threshold);
        // calculate cost
        sa_cost_bfs_step(G, &dist, &path, &avg, &var, &balance);
        cost_new = avg / avg_shortest_path + var / shortest_path_var + balance / path_load_balance;
        delta_cost = cost - cost_new;
        accept_rate = exp(delta_cost * 30000 / t_current) * 100;
        taken = ((rand() % 100) < accept_rate) ? 1 : 0;
        printf("-------- iter %d; t_current = %lf; delta_cost = %lf; cost = %lf, taken = %d; avg = %lf, var = %lf, balance = %lf, P = %lf\n", iter++, t_current, delta_cost, cost_new, taken, avg, var, balance, accept_rate);
        if (taken == 0) // recover
        {
            G->adj_list[node[0]][port[0]] = node[1];
            G->adj_list[node[1]][port[1]] = node[0];
            G->adj_list[node[2]][port[2]] = node[3];
            G->adj_list[node[3]][port[3]] = node[2];
            not_taken_cnt++;
        }
        else
        {
            not_taken_cnt = 0;
            cost = cost_new;
        }
    }
    printf("\n******** End Searching ********\n");
    check_graph_dst_ring(G, dst_threshold);
    check_graph_adjlist(G);
    bfs_delete_path_dist(dist, path);
    free(visit);
    free(stack);
    visit = NULL;
    stack = NULL;
    return G;
}

void sa_cost(GraphAdjList *G, int ***ref_path, unsigned int ***dist, int ***path, double *avg_shortest_path, double *shortest_path_var, double *path_load_balance)
{
    // ref dist & path
    floyd_initial_path_dist(G, dist, ref_path);

    // used dist & path
    int *path_array = (int *)calloc(G->num_nodes * G->num_nodes, sizeof(int));
    assert(path_array != NULL);

    *path = (int **)calloc(G->num_nodes, sizeof(int *));
    assert(*path != NULL);

    for (int i = 0; i < G->num_nodes; ++i)
        (*path)[i] = path_array + i * G->num_nodes;

    memcpy((*path)[0], (*ref_path)[0], G->num_nodes * G->num_nodes * sizeof(int));

    // floyd_algorithm(G->num_nodes, *dist, *path);
    floyd_algorithm_parallel(G->num_nodes, *dist, *path);
    floyd_check_path(G, *path);

    *avg_shortest_path = floyd_average_shortest_path_length(G->num_nodes, *dist);

    *shortest_path_var = floyd_shortest_path_variance(G->num_nodes, *dist);

    unsigned int *array = floyd_initial_path_load_balance(G, *path);
    *path_load_balance = floyd_path_load_balance(G->num_nodes * G->degree, array);

    free(array);
    array = NULL;
    debug("avg %lf, var %lf, balance %lf\n", *avg_shortest_path, *shortest_path_var, *path_load_balance);
}

void sa_cost_bfs(const GraphAdjList *G, unsigned int ***dist, int ***path, double *avg_shortest_path, double *shortest_path_var, double *path_load_balance)
{
    bfs_initial_path_dist(G, dist, path);

    bfs_shortest_path_algorithm_parallel(G, *dist, *path);
    transpose(G->num_nodes, *path);
    floyd_check_path(G, *path);

    *avg_shortest_path = floyd_average_shortest_path_length(G->num_nodes, *dist);

    *shortest_path_var = floyd_shortest_path_variance(G->num_nodes, *dist);

    unsigned int *array = floyd_initial_path_load_balance(G, *path);
    *path_load_balance = floyd_path_load_balance(G->num_nodes * G->degree, array);

    free(array);
    array = NULL;
}

void sa_cost_step(GraphAdjList *G, int ***ref_path, unsigned int ***dist, int ***path, double *avg_shortest_path, double *shortest_path_var, double *path_load_balance)
{
    memcpy((*path)[0], (*ref_path)[0], G->num_nodes * G->num_nodes * sizeof(int));
    memset((*dist)[0], INF, G->num_nodes * G->num_nodes * sizeof(unsigned int));

    for (int i = 0; i < G->num_nodes; ++i)
    {
        (*dist)[i][i] = 0;
        for (int j = 0; j < G->degree; ++j)
        {
            if (G->adj_list[i][j] != INF)
                (*dist)[i][G->adj_list[i][j]] = 1;
        }
    }

    // floyd_initial_path_dist(G, dist, path);
    floyd_algorithm_parallel(G->num_nodes, *dist, *path);

    // floyd_check_path(G, *path);

    *avg_shortest_path = floyd_average_shortest_path_length(G->num_nodes, *dist);

    *shortest_path_var = floyd_shortest_path_variance(G->num_nodes, *dist);

    unsigned int *array = floyd_initial_path_load_balance(G, *path);
    *path_load_balance = floyd_path_load_balance(G->num_nodes * G->degree, array);

    free(array);
    array = NULL;
}

void sa_cost_bfs_step(const GraphAdjList *G, unsigned int ***dist, int ***path, double *avg_shortest_path, double *shortest_path_var, double *path_load_balance)
{

    bfs_shortest_path_algorithm_parallel(G, *dist, *path);
    transpose(G->num_nodes, *path);
    // floyd_check_path(G, *path);

    *avg_shortest_path = floyd_average_shortest_path_length(G->num_nodes, *dist);

    *shortest_path_var = floyd_shortest_path_variance(G->num_nodes, *dist);

    unsigned int *array = floyd_initial_path_load_balance(G, *path);
    *path_load_balance = floyd_path_load_balance(G->num_nodes * G->degree, array);
    free(array);
    array = NULL;
}