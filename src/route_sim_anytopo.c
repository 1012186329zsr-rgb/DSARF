#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <string.h>
#include "route_sim_anytopo.h"
#include "slimfly_routing.h"
#include <stdlib.h>
#include "utils.h"
#include "floyd_warshall.h"
#include "dfs.h"
#include "tree_turn.h"
#include "l_turn.h"
#include "octo_turn.h"
#include "updown_turn.h"
#include "topo_gen.h"
#include "path_with_turn_forbidden.h"
#include <omp.h>

static inline int find_port_to_neighbor_id(const RouterList *R, const Router *r, int neighbor_id)
{
    for (int p = 0; p < R->degree; ++p)
    {
        if (r->neighbor[p] != NULL && r->neighbor[p]->id == neighbor_id)
            return p;
    }
    return -1;
}

static inline unsigned int fast_rand(unsigned int *seed) {
    unsigned int x = *seed;
    if (x == 0) x = 123459876;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *seed = x;
    return x;
}

RouterList *construct_router_list_from_graphadjlist(GraphAdjList *G)
{
    /* proceed with constructing RouterList from G; preserve DIAG ERROR checks below */

    RouterList *R = (RouterList *)calloc(1, sizeof(RouterList));
    assert(R != NULL);
    R->num_nodes = G->num_nodes;
    R->degree = G->degree;
    /* set runtime VC count from global runtime variable */
    R->vc_num = ROUTE_SIM_ANYTOPO_VC_RUNTIME;

    R->rl = (Router *)calloc(R->num_nodes, sizeof(Router));
    assert(R->rl != NULL);
    for (int i = 0; i < R->num_nodes; ++i)
    {
        R->rl[i].id = i;
        R->rl[i].local_o.dst_id = -1;
        R->rl[i].channels = (Channel **)calloc(R->degree, sizeof(Channel *));
        assert(R->rl[i].channels != NULL);
        
        // Contiguous allocation for all channels of this router
        Channel *flat_channels = (Channel *)calloc(R->degree * R->vc_num, sizeof(Channel));
        assert(flat_channels != NULL);

        for (int j = 0; j < R->degree; ++j)
        {
            R->rl[i].channels[j] = flat_channels + j * R->vc_num;
            assert(R->rl[i].channels[j] != NULL);
        }
        /* initialize per-VC structures */
        R->rl[i].lut = (unsigned char ***)calloc(R->vc_num, sizeof(unsigned char **));
        R->rl[i].direction_viable = (unsigned char **)calloc(R->vc_num, sizeof(unsigned char *));
        R->rl[i].self_dst = (unsigned int ***)calloc(R->vc_num, sizeof(unsigned int **));
        R->rl[i].non_local_congestion = (int **)calloc(R->vc_num, sizeof(int *));

        for (int j = 0; j < R->vc_num; ++j) // VC
        {
            for (int k = 0; k < R->degree; ++k)
            {
                R->rl[i].channels[k][j].buffer_o.dst_id = -1;
                if (G->adj_list[i][k] != INF)
                    R->rl[i].channels[k][j].credit = ROUTE_SIM_ANYTOPO_DEPTH;
            }
        }
    }
    for (int i = 0; i < G->num_nodes; ++i)
    {
        R->rl[i].neighbor = (Router **)calloc(G->degree, sizeof(Router *));
        assert(R->rl[i].neighbor != NULL);
        for (int j = 0; j < G->degree; ++j)
        {
            if (G->adj_list[i][j] != INF)
                R->rl[i].neighbor[j] = &(R->rl[G->adj_list[i][j]]);
            else
                R->rl[i].neighbor[j] = NULL;
        }
    }
    int port;
    for (int i = 0; i < G->num_nodes; ++i)
    {
        for (int j = 0; j < G->degree; ++j)
        {
            if (G->adj_list[i][j] != INF && R->rl[i].channels[j][0].connect == NULL)
            {
                port = -1;
                for (int k = 0; k < G->degree; ++k)
                    if (G->adj_list[G->adj_list[i][j]][k] == i)
                    {
                        port = k;
                        break;
                    }
                // assert(port != -1);
                if (port == -1) {
                    fprintf(stderr, "WARNING: Reverse link missing for %d->%d. Skipping connection.\n", i, G->adj_list[i][j]);
                    continue;
                }
                for (int k = 0; k < R->vc_num; ++k)
                {
                        int partner = G->adj_list[i][j];
                        /* Defensive diagnostics */
                        if (partner < 0 || partner >= R->num_nodes) {
                            fprintf(stderr, "DIAG ERROR: invalid partner index for i=%d j=%d partner=%d\n", i, j, partner);
                            continue;
                        }
                        if (port < 0 || port >= R->degree) {
                            fprintf(stderr, "DIAG ERROR: invalid port for i=%d j=%d partner=%d port=%d\n", i, j, partner, port);
                            continue;
                        }
                        if (R->rl[partner].channels == NULL) {
                            fprintf(stderr, "DIAG ERROR: partner channels NULL for i=%d j=%d partner=%d\n", i, j, partner);
                            continue;
                        }
                        if (R->rl[partner].channels[port] == NULL) {
                            fprintf(stderr, "DIAG ERROR: partner channels[port] NULL for i=%d j=%d partner=%d port=%d\n", i, j, partner, port);
                            continue;
                        }
                        if (R->rl[i].channels == NULL) {
                            fprintf(stderr, "DIAG ERROR: self channels NULL for i=%d j=%d\n", i, j);
                            continue;
                        }
                        if (R->rl[i].channels[j] == NULL) {
                            fprintf(stderr, "DIAG ERROR: self channels[j] NULL for i=%d j=%d\n", i, j);
                            continue;
                        }
                    R->rl[i].channels[j][k].connect = &(R->rl[G->adj_list[i][j]].channels[port][k]);
                    R->rl[G->adj_list[i][j]].channels[port][k].connect = &(R->rl[i].channels[j][k]);
                }
            }
        }
    }
    // calloc non_local_congestion initial values
    for (int i = 0; i < R->num_nodes; ++i) // node
    {
        for (int k = 0; k < R->vc_num; ++k)
        {
            R->rl[i].non_local_congestion[k] = (int *)calloc(R->degree, sizeof(int));
            assert(R->rl[i].non_local_congestion[k] != NULL);
            for (int j = 0; j < R->degree; ++j)
                if (G->adj_list[i][j] != INF)
                    R->rl[i].non_local_congestion[k][j] = ROUTE_SIM_ANYTOPO_DEPTH;
        }
    }
    return R;
}


void delete_router_list_traffic(RouterList *R)
{
    for (int i = 0; i < R->num_nodes; ++i)
    {
        if (R->rl[i].local_channel != NULL)
        {
            free(R->rl[i].local_channel);
            R->rl[i].local_channel = NULL;
        }
        R->rl[i].local_size = 0;
        R->rl[i].local_start = 0;
        R->rl[i].local_o.dst_id = -1;
        R->rl[i].recv_num = 0;
        R->rl[i].total_time = 0;
        R->rl[i].max_time = 0;
        for (int k = 0; k < R->degree; ++k)
        {
            for (int j = 0; j < R->vc_num; ++j)
            {
                R->rl[i].channels[k][j].buffer_o.dst_id = -1;
                R->rl[i].channels[k][j].size = 0;
                R->rl[i].channels[k][j].start = 0;
                R->rl[i].channels[k][j].end = 0;
                R->rl[i].channels[k][j].credit = ROUTE_SIM_ANYTOPO_DEPTH;
            }
        }
    }
}

void delete_router_list_lut(RouterList *R)
{
    for (int idx = 0; idx < R->vc_num; ++idx)
    {
        for (int i = 0; i < R->num_nodes; ++i)
        {
            if (R->rl[i].lut[idx] != NULL)
            {
                for (int j = 0; j < R->degree + 1; ++j)
                {
                    free(R->rl[i].lut[idx][j]);
                    R->rl[i].lut[idx][j] = NULL;
                }
                free(R->rl[i].lut[idx]);
                R->rl[i].lut[idx] = NULL;
            }
        }
    }
}

void delete_router_list(RouterList *R)
{
    if (R->sr_len != NULL)
    {
        free(R->sr_len);
        R->sr_len = NULL;
    }
    if (R->sr_nodes != NULL)
    {
        free(R->sr_nodes);
        R->sr_nodes = NULL;
    }
    for (int i = 0; i < R->num_nodes; ++i)
    {
        if (R->rl[i].local_channel != NULL)
        {
            free(R->rl[i].local_channel);
            R->rl[i].local_channel = NULL;
        }
        if (R->rl[i].channels != NULL)
        {
            // Free the contiguous block (pointed to by the first channel pointer)
            if (R->rl[i].channels[0] != NULL)
            {
                free(R->rl[i].channels[0]);
            }
            
            free(R->rl[i].channels);
            R->rl[i].channels = NULL;
        }
    }
    delete_router_list_lut(R);
    free(R->rl);
    R->rl = NULL;
    free(R);
    R = NULL;
}

void init_router_list_traffic(RouterList *R, int **traffic_table)
{
    assert(traffic_table != NULL);
    int send_num;
    for (int i = 0; i < R->num_nodes; ++i)
    {
        send_num = 0;
        for (int j = 0; j < R->num_nodes; ++j)
        {
            if (traffic_table[i][j] > 0)
                send_num++;
        }
        R->rl[i].local_size = send_num;
        // printf("//// send from : %d to \t", i);
        if (send_num > 0)
        {
            R->rl[i].local_channel = (Packets *)calloc(send_num, sizeof(Packets));
            assert(R->rl[i].local_channel != NULL);
            for (int j = 0, k = 0; j < R->num_nodes; ++j)
            {
                if (traffic_table[i][j] > 0)
                {
                    // printf("(%d, %d) ", j, traffic_table[i][j]);
                    R->rl[i].local_channel[k].dst_id = j;
                    R->rl[i].local_channel[k].num = traffic_table[i][j];
                    ++k;
                }
            }
        }
        else
            R->rl[i].local_channel = NULL;
        // printf("\n");
    }
}

int **traffic_uniform_random(const RouterList *R, int p, int packet_num, unsigned int seed)
{
    srand(seed);
    int **traffic_table = (int **)calloc(R->num_nodes, sizeof(int *));
    assert(traffic_table != NULL);
    for (int i = 0; i < R->num_nodes; ++i)
    {
        traffic_table[i] = (int *)calloc(R->num_nodes, sizeof(int));
        assert(traffic_table[i] != NULL);
    }
    for (int i = 0; i < R->num_nodes; ++i)
    {
        for (int j = 0; j < R->num_nodes; ++j)
        {
            if ((rand() % 10000) < p)
                traffic_table[i][j] = packet_num;
        }
    }
    return traffic_table;
}

static int pick_uniform_excluding(unsigned int *seed, int n, int exclude)
{
    if (n <= 1)
        return 0;
    int x = (int)(fast_rand(seed) % (unsigned int)(n - 1));
    return (x >= exclude) ? (x + 1) : x;
}

static int pick_hotspot_excluding(unsigned int *seed, int n, int hotspot_count, int hotspot_base, int exclude)
{
    if (n <= 1)
        return 0;

    if (hotspot_count <= 0)
        return pick_uniform_excluding(seed, n, exclude);
    if (hotspot_count > n)
        hotspot_count = n;

    /* hotspots are fixed nodes: hotspot_base, hotspot_base+1, ... (mod n) */
    int tries = hotspot_count;
    while (tries-- > 0)
    {
        int idx = (int)(fast_rand(seed) % (unsigned int)hotspot_count);
        int dst = (hotspot_base + idx) % n;
        if (dst != exclude)
            return dst;
    }
    return pick_uniform_excluding(seed, n, exclude);
}

static int **traffic_hotspot(const RouterList *R, int p, int packet_num, unsigned int seed, int hot_rate_percent, int hotspot_count, int hotspot_base)
{
    int **traffic_table = (int **)calloc(R->num_nodes, sizeof(int *));
    assert(traffic_table != NULL);
    for (int i = 0; i < R->num_nodes; ++i)
    {
        traffic_table[i] = (int *)calloc(R->num_nodes, sizeof(int));
        assert(traffic_table[i] != NULL);
    }

    if (hot_rate_percent < 0)
        hot_rate_percent = 0;
    if (hot_rate_percent > 100)
        hot_rate_percent = 100;

    /*
     * Hotspot model:
     * - Injection opportunity still controlled by p (like uniform mode)
     * - Of injected traffic, hot_rate_percent goes to fixed hotspot nodes,
     *   the rest follows uniform random destination selection.
     */
    unsigned int s = (seed == 0) ? 123456789u : seed;
    int trials_per_src = (R->num_nodes > 1) ? (R->num_nodes - 1) : 1;
    for (int src = 0; src < R->num_nodes; ++src)
    {
        for (int t = 0; t < trials_per_src; ++t)
        {
            if ((int)(fast_rand(&s) % 10000u) >= p)
                continue;

            int dst;
            int is_hot = ((int)(fast_rand(&s) % 100u) < hot_rate_percent);
            if (is_hot)
                dst = pick_hotspot_excluding(&s, R->num_nodes, hotspot_count, hotspot_base, src);
            else
                dst = pick_uniform_excluding(&s, R->num_nodes, src);

            traffic_table[src][dst] += packet_num;
        }
    }
    return traffic_table;
}

static int **traffic_all_to_all(const RouterList *R, int packet_num)
{
    int **traffic_table = (int **)calloc(R->num_nodes, sizeof(int *));
    assert(traffic_table != NULL);
    for (int i = 0; i < R->num_nodes; ++i)
    {
        traffic_table[i] = (int *)calloc(R->num_nodes, sizeof(int));
        assert(traffic_table[i] != NULL);
    }
    for (int i = 0; i < R->num_nodes; ++i)
    {
        for (int j = 0; j < R->num_nodes; ++j)
        {
            traffic_table[i][j] = (i == j) ? 0 : packet_num;
        }
    }
    return traffic_table;
}

static void traffic_table_delete(int **traffic_table, int num)
{
    if (traffic_table == NULL) return;
    for (int i = 0; i < num; ++i)
    {
        free(traffic_table[i]);
    }
    free(traffic_table);
}

int **traffic_from_file(char *file_name, int num)
{
    // alloc
    int **traffic_table = (int **)calloc(num, sizeof(int *));
    assert(traffic_table != NULL);
    for (int i = 0; i < num; ++i)
    {
        traffic_table[i] = (int *)calloc(num, sizeof(int));
        assert(traffic_table[i] != NULL);
    }
    // read
    FILE *f = fopen(file_name, "r");
    assert(f != NULL);
    char *one_line = (char *)calloc(num * 15, sizeof(char));
    assert(one_line != NULL);
    char *token;
    int temp;
    for (int i = 0; i < num; ++i)
    {
        assert(fgets(one_line, sizeof(char) * num * 15, f) != NULL);
        token = strtok(one_line, ",");
        int j = 0;
        while (token != NULL)
        {
            temp = atoi(token);
            assert(temp >= 0);
            traffic_table[i][j] = temp;
            token = strtok(NULL, ",");
            ++j;
        }
    }
    fclose(f);
    f = NULL;
    free(one_line);
    one_line = NULL;
    token = NULL;
    return traffic_table;
}

void init_router_list_lut_shortest_path(RouterList *R, const GraphAdjList *G, int root_select, int (*turn_check)(int, int, int, int, int, int), unsigned char *port_map)
{
    // Generate root & TC using runtime VC count
    int vc = R->vc_num;
    TreeCoordinate **tc = (TreeCoordinate **)calloc(vc, sizeof(TreeCoordinate *));
    assert(tc != NULL);
    int *root = (int *)calloc(vc, sizeof(int));
    assert(root != NULL);
    if (root_select == 0) // random select one root, and apply it to every VC
    {
        int best_root = rand() % G->num_nodes;
        // allow forcing root via environment variable FORCE_ROOT
        char *env_root = getenv("FORCE_ROOT");
        if (env_root != NULL)
        {
            int r = atoi(env_root);
            if (r >= 0 && r < G->num_nodes)
            {
                best_root = r;
                printf("FORCE_ROOT used: %d\n", best_root);
            }
            else
            {
                printf("FORCE_ROOT=%d out of range, ignoring\n", r);
            }
        }
        for (int i = 0; i < vc; ++i)
        {
            root[i] = best_root;
            tc[i] = gen_tree_coordinate(G, root[i], 1);
        }
    }
    else if (root_select == 1) // random select one root for every VC independently
    {
        for (int i = 0; i < vc; ++i)
        {
            root[i] = rand() % G->num_nodes;
            tc[i] = gen_tree_coordinate(G, root[i], 1);
        }
    }
    else if (root_select == 2) // optimally select roots for every VC
    {
        root = root_selection(G, turn_check, vc);
        for (int i = 0; i < vc; ++i)
        {
            tc[i] = gen_tree_coordinate(G, root[i], 1);
            printf("\t best root is : %d", root[i]);
        }
        printf("\n");
    }
    else
        assert(0);

    // generate LUT
    delete_router_list_lut(R);
    // find shortest path
    for (int i = 0; i < R->num_nodes; ++i)
    {
        memset(port_map + i * R->num_nodes, 255, R->num_nodes * sizeof(unsigned char));
        for (unsigned char j = 0; j < G->degree; ++j)
        {
            if (G->adj_list[i][j] != INF)
            {
                port_map[i * R->num_nodes + G->adj_list[i][j]] = j; // dst[i][p] = j; means: node i's j-th port is connected to node p;
            }
        }
    }
    trun_model_bfs_new(G, tc, R, turn_check);

    for (int k = 0; k < vc; ++k)
    {
        // check lut
        int false_path_num = 0;
        for (int core = 0; core < R->num_nodes; ++core)
        {
            for (int i = 0; i < R->degree + 1; ++i)
            {
                for (int j = 0; j < R->num_nodes; ++j)
                {
                    if (R->rl[core].lut[k][i][j] == 255)
                    {
                        assert(R->rl[core].self_dst[k][i][j] == INF);
                        false_path_num++;
                    }
                    else
                    {
                        assert(R->rl[core].self_dst[k][i][j] != INF);
                    }
                    if (i == R->degree)
                    {
                        assert(R->rl[core].lut[k][i][j] != 255);
                        assert(R->rl[core].self_dst[k][i][j] != INF);
                    }
                }
            }
        }
        double false_dist_rate = (double)false_path_num / (double)(G->degree * G->num_nodes * G->num_nodes);
        printf("False dist rate is %lf\n", false_dist_rate);

        // check lut & self_dst by path
        int cur_dst = 0;
        int cur_node = 0;
        int out_port = 0;
        int next_node = 0;
        int in_port = 0;
        for (int i = 0; i < R->num_nodes; ++i) // src node
        {
            for (int j = 0; j < R->num_nodes; ++j) // dst node
            {
                if (i == j)
                {
                    assert(R->rl[i].lut[k][R->degree][j] == 254);
                    assert(R->rl[i].self_dst[k][R->degree][j] == 0);
                }
                else
                {
                    cur_node = i;
                    cur_dst = R->rl[cur_node].self_dst[k][R->degree][j];
                    out_port = R->rl[cur_node].lut[k][R->degree][j];
                    next_node = G->adj_list[cur_node][out_port];
                    in_port = port_map[next_node * R->num_nodes + cur_node];
                    while (next_node != j)
                    {
                        if (in_port >= R->degree) break;
                        assert(R->rl[next_node].self_dst[k][in_port][j] == cur_dst - 1);
                        cur_dst--;
                        cur_node = next_node;
                        out_port = R->rl[cur_node].lut[k][in_port][j];
                        next_node = G->adj_list[cur_node][out_port];
                        in_port = port_map[next_node * R->num_nodes + cur_node];
                    }
                    if (in_port < R->degree) {
                        assert(R->rl[j].lut[k][in_port][j] == 254);
                        assert(R->rl[j].self_dst[k][in_port][j] == 0);
                    }
                }
            }
        }

        // generate direction_viable
        unsigned int *neighbors = NULL;
        for (int i = 0; i < R->num_nodes; ++i) // node
        {
            R->rl[i].direction_viable[k] = (unsigned char *)calloc(R->degree, sizeof(unsigned char));
            assert(R->rl[i].direction_viable[k] != NULL);
            for (int j = 0; j < R->degree; ++j) // input port
            {
                for (int p = 0; p < R->degree; // output port
                ++p)
                {
                    neighbors = G->adj_list[i];
                    if (j == p || neighbors[j] == INF || neighbors[p] == INF)
                        continue;
                    if ((*turn_check)(tc[k]->x[neighbors[j]], tc[k]->y[neighbors[j]], tc[k]->x[i], tc[k]->y[i], tc[k]->x[neighbors[p]], tc[k]->y[neighbors[p]]) != 0)
                        R->rl[i].direction_viable[k][j] |= (1 << p); // bitmap
                }
            }
        }
        neighbors = NULL;
    }
    for (int i = 0; i < vc; ++i)
        delete_tc(tc[i]);
    free(tc);
    free(root);
    printf("--------------------- End of LUT INIT -------------------------\n");
}

void init_router_list_lut_weighted_constrained(RouterList *R, const GraphAdjList *G, unsigned char *port_map)
{
    printf("Initializing LUT with Weighted Constrained Routing (Hop <= 3)...\n");
    
    int num_nodes = G->num_nodes;
    int degree = G->degree;

    /* (Re)allocate source-route tables */
    if (R->sr_len != NULL)
    {
        free(R->sr_len);
        R->sr_len = NULL;
    }
    if (R->sr_nodes != NULL)
    {
        free(R->sr_nodes);
        R->sr_nodes = NULL;
    }
    R->sr_len = (unsigned char *)calloc(num_nodes * num_nodes, sizeof(unsigned char));
    assert(R->sr_len != NULL);
    R->sr_nodes = (int *)calloc((size_t)num_nodes * (size_t)num_nodes * (ROUTE_SIM_ANYTOPO_DEPTH + 1), sizeof(int));
    assert(R->sr_nodes != NULL);
    
    // 1. Initialize link weights (load counters)
    // link_load[u][v] stores the number of paths using link u->v
    // Since G is adjacency list, we can use a flat array or map if N is large, 
    // but for N~3000, N*N int array is ~36MB, which is fine.
    int *link_load = (int *)calloc(num_nodes * num_nodes, sizeof(int));
    assert(link_load != NULL);

    // Helper to get weight
    #define GET_WEIGHT(u, v) (link_load[(u) * num_nodes + (v)])
    #define INC_WEIGHT(u, v) (link_load[(u) * num_nodes + (v)]++)

    // Helper to check connectivity and get port
    // Returns port index if u->v exists, else -1
    auto int get_port_idx(int u, int v) {
        for (int k = 0; k < degree; ++k) {
            if (G->adj_list[u][k] == v) return k;
        }
        return -1;
    };

    // Clear existing LUTs
    delete_router_list_lut(R);
    // Re-allocate LUTs
    for (int i = 0; i < num_nodes; ++i) {
        R->rl[i].lut = (unsigned char ***)calloc(R->vc_num, sizeof(unsigned char **));
        R->rl[i].self_dst = (unsigned int ***)calloc(R->vc_num, sizeof(unsigned int **));
        R->rl[i].direction_viable = (unsigned char **)calloc(R->vc_num, sizeof(unsigned char *));
        R->rl[i].non_local_congestion = (int **)calloc(R->vc_num, sizeof(int *));
        
        for (int vc = 0; vc < R->vc_num; ++vc) {
            R->rl[i].lut[vc] = (unsigned char **)calloc(degree + 1, sizeof(unsigned char *));
            R->rl[i].self_dst[vc] = (unsigned int **)calloc(degree + 1, sizeof(unsigned int *));
            R->rl[i].direction_viable[vc] = (unsigned char *)calloc(degree, sizeof(unsigned char));
            R->rl[i].non_local_congestion[vc] = (int *)calloc(degree, sizeof(int));

            for (int p = 0; p <= degree; ++p) {
                R->rl[i].lut[vc][p] = (unsigned char *)calloc(num_nodes, sizeof(unsigned char));
                R->rl[i].self_dst[vc][p] = (unsigned int *)calloc(num_nodes, sizeof(unsigned int));
                for(int n=0; n<num_nodes; ++n) {
                    R->rl[i].lut[vc][p][n] = 255; // Invalid port
                    R->rl[i].self_dst[vc][p][n] = INF;
                }
            }
            // Self-to-self distance is 0
            R->rl[i].self_dst[vc][degree][i] = 0;
            R->rl[i].lut[vc][degree][i] = 254; // Local port
        }
    }

    // 2. Iterate over all S-D pairs
    for (int s = 0; s < num_nodes; ++s) {
        for (int d = 0; d < num_nodes; ++d) {
            if (s == d) continue;

            // Find best path with max hop <= 3
            // Path structure: s -> n1 -> n2 -> d (up to 3 hops)
            // We store the best next hop from s
            int best_next_hop = -1;
            int min_total_load = 2147483647; // INT_MAX
            int best_hop_count = 0;
            
            // Path reconstruction info
            int path_nodes[4]; // s, n1, n2, d
            int path_len = 0;

            // A. Check 1-hop: s -> d
            int p_sd = get_port_idx(s, d);
            if (p_sd != -1) {
                int load = GET_WEIGHT(s, d);
                if (load < min_total_load) {
                    min_total_load = load;
                    best_next_hop = d;
                    best_hop_count = 1;
                    path_nodes[0]=s; path_nodes[1]=d;
                    path_len = 2;
                }
            }

            // B. Check 2-hop: s -> m -> d
            for (int k = 0; k < degree; ++k) {
                int m = G->adj_list[s][k];
                if (m == INF) continue;
                if (m == d) continue; // Already checked in 1-hop

                int p_md = get_port_idx(m, d);
                if (p_md != -1) {
                    int load = GET_WEIGHT(s, m) + GET_WEIGHT(m, d);
                    if (load < min_total_load) {
                        min_total_load = load;
                        best_next_hop = m;
                        best_hop_count = 2;
                        path_nodes[0]=s; path_nodes[1]=m; path_nodes[2]=d;
                        path_len = 3;
                    }
                }
            }

            // C. Check 3-hop: s -> a -> b -> d
            // Only if we haven't found a very low load path yet, or to ensure global balance
            // To optimize, we iterate neighbors of s (a) and neighbors of d (b)
            for (int k1 = 0; k1 < degree; ++k1) {
                int a = G->adj_list[s][k1];
                if (a == INF || a == d) continue;

                for (int k2 = 0; k2 < degree; ++k2) {
                    int b = G->adj_list[d][k2]; // Backward from d to find b
                    // Note: adj_list is undirected graph usually, so b->d implies d->b exists
                    if (b == INF || b == s || b == a) continue;

                    // Check if a -> b exists
                    if (get_port_idx(a, b) != -1) {
                        int load = GET_WEIGHT(s, a) + GET_WEIGHT(a, b) + GET_WEIGHT(b, d);
                        if (load < min_total_load) {
                            min_total_load = load;
                            best_next_hop = a;
                            best_hop_count = 3;
                            path_nodes[0]=s; path_nodes[1]=a; path_nodes[2]=b; path_nodes[3]=d;
                            path_len = 4;
                        }
                    }
                }
            }

            // Apply best path
            if (best_next_hop != -1) {
                if (path_len < 2 || path_len > (ROUTE_SIM_ANYTOPO_DEPTH + 1))
                {
                    fprintf(stderr, "WARNING: invalid path_len=%d for %d -> %d\n", path_len, s, d);
                    continue;
                }

                /* Save the full path for Source Routing (scheme A) */
                R->sr_len[s * num_nodes + d] = (unsigned char)path_len;
                int base = (s * num_nodes + d) * (ROUTE_SIM_ANYTOPO_DEPTH + 1);
                for (int t = 0; t < path_len; ++t)
                    R->sr_nodes[base + t] = path_nodes[t];

                int port = get_port_idx(s, best_next_hop);
                // Update LUT for all VCs (or specific ones if needed)
                for (int vc = 0; vc < R->vc_num; ++vc) {
                    // Update LUT for injection (input port = degree)
                    R->rl[s].lut[vc][degree][d] = port;
                    R->rl[s].self_dst[vc][degree][d] = best_hop_count;
                    
                    // Also need to update intermediate nodes' LUTs?
                    // The standard way is to run this for every node pair independently.
                    // But here we are iterating s, d.
                    // For source routing or path setup, we update weights now.
                    // For hop-by-hop LUT, each node needs to know its next hop to d.
                    // Since we iterate all s, we cover all "current nodes".
                    
                    // However, for consistency, we should only set the LUT at 's'.
                    // The next hop 'best_next_hop' will have its own entry for 'd' computed when s=best_next_hop.
                    // WAIT: This greedy approach (Dijkstra-like) requires consistency.
                    // If s->a->b->d is best for s->d, then a->b->d MUST be best for a->d?
                    // Not necessarily with dynamic weights! This is a potential issue with hop-by-hop routing.
                    // If we use source routing (path setup), it's fine.
                    // But here we fill LUT. If s chooses 'a', but 'a' chooses 'c' to go to 'd', we have a problem.
                    // BUT: The paper implies "insertion of a new path". This usually implies circuit switching or source routing.
                    // For hop-by-hop, we can only approximate this by hoping the weight updates lead to consistent decisions.
                    // OR: We assume the weights are global and static after calculation.
                    
                    // For this implementation, we will just set the next hop for s->d.
                    // And update weights for the WHOLE path we found.
                }

                // Update weights for the chosen path
                for (int i = 0; i < path_len - 1; ++i) {
                    int u = path_nodes[i];
                    int v = path_nodes[i+1];
                    INC_WEIGHT(u, v);
                    // Assuming symmetric links for load balancing?
                    // Usually traffic is directional.
                }
            } else {
                fprintf(stderr, "WARNING: No path <= 3 hops found for %d -> %d\n", s, d);
            }
        }
    }
    
    // Post-processing: Fill self_dst and direction_viable based on the generated LUT
    // Since we only filled lut[vc][degree][d] (injection), we need to propagate or fill for transit.
    // Actually, for hop-by-hop, we need lut[vc][input_port][d].
    // Assuming input port doesn't matter for the routing decision (destination based),
    // we can copy injection LUT to all input ports.
    for (int i = 0; i < num_nodes; ++i) {
        for (int vc = 0; vc < R->vc_num; ++vc) {
            for (int p = 0; p < degree; ++p) {
                for (int d = 0; d < num_nodes; ++d) {
                    R->rl[i].lut[vc][p][d] = R->rl[i].lut[vc][degree][d];
                    R->rl[i].self_dst[vc][p][d] = R->rl[i].self_dst[vc][degree][d];
                }
                // Enable all directions for now (no turn restrictions in this mode, relying on hop count)
                // Or we should calculate viable directions based on the paths we chose?
                // For simplicity, enable all connected ports.
                R->rl[i].direction_viable[vc][p] = 0;
                for (int out = 0; out < degree; ++out) {
                    if (R->rl[i].neighbor[out] != NULL) {
                        R->rl[i].direction_viable[vc][p] |= (1 << out);
                    }
                }
            }
        }
    }

    free(link_load);
    printf("Weighted Constrained Routing Init Done.\n");
}

double avg_packet_delivery_latency(const RouterList *R)
{
    unsigned long long total_time = 0;
    long long total_packet_num = 0;
    for (int i = 0; i < R->num_nodes; ++i)
    {
        total_time += R->rl[i].total_time;
        assert(total_time < (1ULL << 62)); // in case of overflow
        total_packet_num += R->rl[i].recv_num;
    }
    return (double)((long double)total_time / (long double)total_packet_num);
}

unsigned int max_packet_delivery_latency(const RouterList *R)
{
    unsigned int max_latency = 0;
    for (int i = 0; i < R->num_nodes; ++i)
    {
        if (R->rl[i].max_time > max_latency)
            max_latency = R->rl[i].max_time;
    }
    return max_latency;
}

void run_simulation_parallel(RouterList *R, unsigned int *sim_time_ptr, int path_diversity_mode, int load_balance_mode, unsigned char *port_map, unsigned int *path_log, int ppp, int packets_num, int route_lut_mode, unsigned long long total_packets)
{
    int stop_flag = 0;
    int dead_lock_cnt = 0;
    unsigned int sim_time = *sim_time_ptr;
    int router_empty = 0;
    int router_busy = 0;
    int l_empty, l_busy;
    unsigned int base_seed;

    #pragma omp parallel
    {
        int p_dst_id, p_src_id;
        unsigned int p_start_time;
        int p_ttl;
        int p_val_mid;
        unsigned char p_val_phase;
        Router *r = NULL;
        unsigned char dst_port;
        int dst_vc;
        int viable_output_num;
        int viable_output_list[128];
        unsigned int seed;

        while (!stop_flag)
        {
            #pragma omp single
            {
                l_empty = 1;
                l_busy = 0;
                base_seed = rand();
                if (base_seed == 0) base_seed = 1;
            }
            seed = base_seed + omp_get_thread_num();

            // --- router_arbit part ---
            #pragma omp for reduction(min:l_empty) reduction(max:l_busy)
            for (int node = 0; node < R->num_nodes; ++node)
            {
                r = &(R->rl[node]);
                for (int i = 0; i < R->degree; ++i)
                {
                    for (int j = 0; j < R->vc_num; ++j)
                    {
                        if (r->channels[i][j].size == 0)
                            continue;

                        l_empty = 0;
                        Packet *head = &r->channels[i][j].buffer_i[r->channels[i][j].start];
                        p_src_id = head->src_id;
                        p_dst_id = head->dst_id;
                        p_start_time = head->start_time;
                        p_ttl = head->ttl;
                        p_val_mid = head->val_mid;
                        p_val_phase = head->val_phase;

                        dst_port = 255;
                        int route_dst = p_dst_id;

                        /* Valiant two-phase (only for non-source-routing modes)
                           path_diversity_mode==2: always valiant
                           path_diversity_mode==3: UGAL-L may choose valiant per packet (val_mid set at injection)
                        */
                        if (!(route_lut_mode == 4 && head->sr_len > 0) && (path_diversity_mode == 2 || path_diversity_mode == 3) && p_val_mid != -1)
                        {
                            if (p_val_phase == 0)
                            {
                                if (p_val_mid == node)
                                {
                                    p_val_phase = 1;
                                    route_dst = p_dst_id;
                                }
                                else
                                {
                                    route_dst = p_val_mid;
                                }
                            }
                        }

                        /* route_lut_mode==4: strict source routing if sr_len enabled */
                        if (route_lut_mode == 4 && head->sr_len > 0)
                        {
                            if (head->sr_pos + 1 < head->sr_len)
                            {
                                int next_node_id = head->sr_nodes[head->sr_pos + 1];
                                int outp = find_port_to_neighbor_id(R, r, next_node_id);
                                if (outp >= 0)
                                    dst_port = (unsigned char)outp;
                            }
                        }
                        else if (route_dst != node)
                        {
                            if (path_diversity_mode == -1)
                            {
                                dst_port = r->lut[j][i][route_dst];
                            }
                            else if (path_diversity_mode == 0 || path_diversity_mode == 2 || path_diversity_mode == 3)
                            {
                                int shortest_dst = r->self_dst[j][i][route_dst];
                                if (load_balance_mode == 0)
                                {
                                    viable_output_num = 0;
                                    for (int output_idx = 0; output_idx < R->degree; ++output_idx)
                                    {
                                        if (r->neighbor[output_idx] == NULL) continue;
                                        int neighbor_port = port_map[r->neighbor[output_idx]->id * R->num_nodes + r->id];
                                        if (neighbor_port >= R->degree) continue;
                                        unsigned int ndst = (route_lut_mode == 4)
                                                            ? r->neighbor[output_idx]->self_dst[j][R->degree][route_dst]
                                                            : r->neighbor[output_idx]->self_dst[j][neighbor_port][route_dst];
                                        if (ndst == (unsigned int)(shortest_dst - 1))
                                            viable_output_list[viable_output_num++] = output_idx;
                                    }
                                    if (viable_output_num > 0)
                                        dst_port = (unsigned char)viable_output_list[fast_rand(&seed) % viable_output_num];
                                    else
                                        dst_port = r->lut[j][i][route_dst];
                                }
                                else if (load_balance_mode == 1)
                                {
                                    int biggest_credit = 0;
                                    viable_output_num = 0;
                                    for (int output_idx = 0; output_idx < R->degree; ++output_idx)
                                    {
                                        if (r->neighbor[output_idx] == NULL) continue;
                                        int neighbor_port = port_map[r->neighbor[output_idx]->id * R->num_nodes + r->id];
                                        if (neighbor_port >= R->degree) continue;
                                        unsigned int ndst = (route_lut_mode == 4)
                                                            ? r->neighbor[output_idx]->self_dst[j][R->degree][route_dst]
                                                            : r->neighbor[output_idx]->self_dst[j][neighbor_port][route_dst];
                                        if (ndst == (unsigned int)(shortest_dst - 1))
                                        {
                                            if (biggest_credit < r->channels[output_idx][j].credit)
                                            {
                                                viable_output_num = 0;
                                                viable_output_list[viable_output_num++] = output_idx;
                                                biggest_credit = r->channels[output_idx][j].credit;
                                            }
                                            else if (biggest_credit == r->channels[output_idx][j].credit && biggest_credit != 0)
                                            {
                                                viable_output_list[viable_output_num++] = output_idx;
                                            }
                                        }
                                    }
                                    if (viable_output_num > 0)
                                        dst_port = (unsigned char)viable_output_list[fast_rand(&seed) % viable_output_num];
                                    else
                                        dst_port = r->lut[j][i][route_dst];
                                }
                                else if (load_balance_mode == 2)
                                {
                                    int biggest_credit = 0;
                                    viable_output_num = 0;
                                    for (int output_idx = 0; output_idx < R->degree; ++output_idx)
                                    {
                                        if (r->neighbor[output_idx] == NULL) continue;
                                        int neighbor_port = port_map[r->neighbor[output_idx]->id * R->num_nodes + r->id];
                                        if (neighbor_port >= R->degree) continue;
                                        unsigned int ndst = (route_lut_mode == 4)
                                                            ? r->neighbor[output_idx]->self_dst[j][R->degree][route_dst]
                                                            : r->neighbor[output_idx]->self_dst[j][neighbor_port][route_dst];
                                        if (ndst == (unsigned int)(shortest_dst - 1) && r->channels[output_idx][j].buffer_o.dst_id == -1)
                                        {
                                            if (biggest_credit < r->non_local_congestion[j][output_idx])
                                            {
                                                viable_output_num = 0;
                                                viable_output_list[viable_output_num++] = output_idx;
                                                biggest_credit = r->non_local_congestion[j][output_idx];
                                            }
                                            else if (biggest_credit == r->non_local_congestion[j][output_idx] && biggest_credit != 0)
                                            {
                                                viable_output_list[viable_output_num++] = output_idx;
                                            }
                                        }
                                    }
                                    if (viable_output_num > 0)
                                        dst_port = (unsigned char)viable_output_list[fast_rand(&seed) % viable_output_num];
                                    else
                                        dst_port = r->lut[j][i][route_dst];
                                }
                                else
                                {
                                    assert(0);
                                }
                            }
                            else
                            {
                                /* non-minimal diversity: keep legacy behavior (load_balance_mode!=0) */
                                int shortest_dst = r->self_dst[j][i][route_dst];
                                if (load_balance_mode == 0)
                                {
                                    /* degrade to minimal ECMP */
                                    viable_output_num = 0;
                                    for (int output_idx = 0; output_idx < R->degree; ++output_idx)
                                    {
                                        if (r->neighbor[output_idx] == NULL) continue;
                                        int neighbor_port = port_map[r->neighbor[output_idx]->id * R->num_nodes + r->id];
                                        if (neighbor_port >= R->degree) continue;
                                        if (r->neighbor[output_idx]->self_dst[j][neighbor_port][route_dst] == (unsigned int)(shortest_dst - 1))
                                            viable_output_list[viable_output_num++] = output_idx;
                                    }
                                    if (viable_output_num > 0)
                                        dst_port = (unsigned char)viable_output_list[fast_rand(&seed) % viable_output_num];
                                    else
                                        dst_port = r->lut[j][i][route_dst];
                                }
                                else
                                {
                                    /* keep original non-minimal selection (copied from earlier versions) */
                                    int non_minimal_biggest_credit = 0;
                                    int non_minimal_best_port = -1;
                                    int minimal_biggest_credit = 0;
                                    viable_output_num = 0;
                                    for (int output_idx = 0; output_idx < R->degree; ++output_idx)
                                    {
                                        if (r->neighbor[output_idx] == NULL) continue;
                                        int neighbor_port = port_map[r->neighbor[output_idx]->id * R->num_nodes + r->id];
                                        if (neighbor_port >= R->degree) continue;
                                        if (r->neighbor[output_idx]->self_dst[j][neighbor_port][route_dst] == (unsigned int)(shortest_dst - 1))
                                        {
                                            if (minimal_biggest_credit < r->channels[output_idx][j].credit)
                                            {
                                                viable_output_num = 0;
                                                viable_output_list[viable_output_num++] = output_idx;
                                                minimal_biggest_credit = r->channels[output_idx][j].credit;
                                            }
                                            else if (minimal_biggest_credit == r->channels[output_idx][j].credit && minimal_biggest_credit != 0)
                                            {
                                                viable_output_list[viable_output_num++] = output_idx;
                                            }
                                        }
                                        else
                                        {
                                            if (neighbor_port < R->degree && r->neighbor[output_idx]->self_dst[j][neighbor_port][route_dst] != INF)
                                            {
                                                int score = (load_balance_mode == 2) ? r->non_local_congestion[j][output_idx] : r->channels[output_idx][j].credit;
                                                if (non_minimal_biggest_credit < score)
                                                {
                                                    non_minimal_biggest_credit = score;
                                                    non_minimal_best_port = output_idx;
                                                }
                                                else if (non_minimal_biggest_credit == score && non_minimal_biggest_credit != 0)
                                                {
                                                    if (fast_rand(&seed) % 2 == 0)
                                                        non_minimal_best_port = output_idx;
                                                }
                                            }
                                        }
                                    }

                                    if (p_ttl > 0 && non_minimal_best_port != -1 && minimal_biggest_credit == 0)
                                    {
                                        dst_port = (unsigned char)non_minimal_best_port;
                                        p_ttl--;
                                    }
                                    else
                                    {
                                        if (viable_output_num > 0)
                                            dst_port = (unsigned char)viable_output_list[fast_rand(&seed) % viable_output_num];
                                        else
                                            dst_port = r->lut[j][i][route_dst];
                                    }
                                }
                            }
                        }

                        /* deliver / forward */
                        int sr_at_final = 1;
                        if (route_lut_mode == 4 && head->sr_len > 0)
                            sr_at_final = (head->sr_pos == (unsigned char)(head->sr_len - 1));

                        int at_final_dest;
                        if (route_lut_mode == 4 && head->sr_len > 0)
                            at_final_dest = (p_dst_id == node && sr_at_final);
                        else
                            at_final_dest = (p_dst_id == node && !(route_lut_mode != 4 && (path_diversity_mode == 2 || path_diversity_mode == 3) && p_val_phase == 0));

                        if (at_final_dest && r->local_o.dst_id == -1)
                        {
                            r->local_o.src_id = p_src_id;
                            r->local_o.dst_id = p_dst_id;
                            r->local_o.start_time = p_start_time;
                            r->local_o.ttl = p_ttl;
                            r->local_o.val_mid = p_val_mid;
                            r->local_o.val_phase = p_val_phase;
                            r->local_o.sr_len = head->sr_len;
                            r->local_o.sr_pos = head->sr_pos;
                            memcpy(r->local_o.sr_nodes, head->sr_nodes, sizeof(r->local_o.sr_nodes));

                            r->channels[i][j].size -= 1;
                            r->channels[i][j].start = (r->channels[i][j].start + 1) % ROUTE_SIM_ANYTOPO_DEPTH;
                            l_busy = 1;

                            r->recv_num++;
                            unsigned int latency = sim_time - p_start_time + 1;
                            r->total_time += latency;
                            if (latency > r->max_time) r->max_time = latency;
                            assert(r->total_time < (1ULL << 62));
                        }
                        else if (dst_port < R->degree && !at_final_dest)
                        {
                            int next_vc = j;
                            if (route_lut_mode == 4)
                            {
                                next_vc = j + 1;
                                if (next_vc >= R->vc_num)
                                    next_vc = -1;
                            }
                            if (next_vc != -1 && r->channels[dst_port][next_vc].buffer_o.dst_id == -1 && r->channels[dst_port][next_vc].credit > 0)
                            {
                                Packet *out = &r->channels[dst_port][next_vc].buffer_o;
                                out->src_id = p_src_id;
                                out->dst_id = p_dst_id;
                                out->start_time = p_start_time;
                                out->ttl = p_ttl;
                                out->val_mid = p_val_mid;
                                out->val_phase = p_val_phase;
                                out->sr_len = head->sr_len;
                                out->sr_pos = (route_lut_mode == 4 && head->sr_len > 0) ? (unsigned char)(head->sr_pos + 1) : head->sr_pos;
                                memcpy(out->sr_nodes, head->sr_nodes, sizeof(out->sr_nodes));

                                r->channels[dst_port][next_vc].credit -= 1;
                                r->channels[i][j].size -= 1;
                                r->channels[i][j].start = (r->channels[i][j].start + 1) % ROUTE_SIM_ANYTOPO_DEPTH;
                                l_busy = 1;
                                path_log[r->id * R->degree + dst_port] += 1;
                            }
                        }
                    }
                }

                /* injection */
                if (r->local_start < r->local_size)
                {
                    l_empty = 0;
                    p_src_id = node;
                    p_dst_id = r->local_channel[r->local_start].dst_id;

                    p_val_mid = -1;
                    p_val_phase = 1;
                    int inject_route_dst = p_dst_id;

                    int sr_nodes_local[ROUTE_SIM_ANYTOPO_DEPTH + 1] = {0};
                    unsigned char sr_len_local = 0;

                    /* Choose injection VC first (UGAL-L cost uses local occupancy on that VC). */
                    if (route_lut_mode == 4)
                        dst_vc = 0;
                    else
                        dst_vc = fast_rand(&seed) % R->vc_num;

                    if ((path_diversity_mode == 2 || path_diversity_mode == 3) && p_dst_id != node)
                    {
                        int choose_valiant = (path_diversity_mode == 2);
                        int mid = (int)(fast_rand(&seed) % R->num_nodes);

                        /* Pure UGAL-L only for non-source-routing modes.
                           In SR mode (route_lut_mode==4), keep legacy valiant-only behavior (mode 2) and
                           treat mode 3 as minimal (choose_valiant stays 0).
                        */
                        if (path_diversity_mode == 3 && route_lut_mode != 4)
                        {
                            /* choose a mid != src,dst when possible */
                            if (R->num_nodes > 2)
                            {
                                int attempts = 0;
                                while ((mid == node || mid == p_dst_id) && attempts < 20)
                                {
                                    mid = (int)(fast_rand(&seed) % R->num_nodes);
                                    attempts++;
                                }
                            }
                            else
                            {
                                mid = p_dst_id;
                            }

                            /* distance estimate (use the selected VC and injection state input=degree) */
                            unsigned int dist_min = r->self_dst[dst_vc][R->degree][p_dst_id];
                            unsigned int dist1 = r->self_dst[dst_vc][R->degree][mid];
                            unsigned int dist2 = R->rl[mid].self_dst[dst_vc][R->degree][p_dst_id];

                            /* local occupancy estimate: use downstream buffer occupancy on chosen output port */
                            unsigned char port_min = r->lut[dst_vc][R->degree][p_dst_id];
                            unsigned char port_val = r->lut[dst_vc][R->degree][mid];
                            int q_min = ROUTE_SIM_ANYTOPO_DEPTH;
                            int q_val = ROUTE_SIM_ANYTOPO_DEPTH;
                            if (port_min < R->degree)
                                q_min = ROUTE_SIM_ANYTOPO_DEPTH - r->channels[port_min][dst_vc].credit;
                            if (port_val < R->degree)
                                q_val = ROUTE_SIM_ANYTOPO_DEPTH - r->channels[port_val][dst_vc].credit;

                            if (dist_min == INF)
                            {
                                choose_valiant = 1;
                            }
                            else if (dist1 == INF || dist2 == INF || mid == p_dst_id)
                            {
                                choose_valiant = 0;
                            }
                            else
                            {
                                /* alpha = (DEPTH+1): simple, dimensionless scaling */
                                unsigned int cost_min = (unsigned int)(q_min * (ROUTE_SIM_ANYTOPO_DEPTH + 1)) + dist_min;
                                unsigned int cost_val = (unsigned int)(q_val * (ROUTE_SIM_ANYTOPO_DEPTH + 1)) + dist1 + dist2;
                                choose_valiant = (cost_val < cost_min);
                            }
                        }

                        if (choose_valiant)
                            p_val_phase = 0;

                        if (route_lut_mode == 4)
                        {
                            int attempts = 0;
                            int valid_mid = 0;
                            while (attempts < 20)
                            {
                                if (mid != node && mid != p_dst_id)
                                {
                                    unsigned char l1 = R->sr_len[node * R->num_nodes + mid];
                                    unsigned char l2 = R->sr_len[mid * R->num_nodes + p_dst_id];
                                    if (l1 >= 2 && l2 >= 2)
                                    {
                                        int hops = (int)(l1 - 1) + (int)(l2 - 1);
                                        if (hops <= ROUTE_SIM_ANYTOPO_DEPTH)
                                        {
                                            valid_mid = 1;
                                            break;
                                        }
                                    }
                                }
                                mid = (int)(fast_rand(&seed) % R->num_nodes);
                                attempts++;
                            }
                            if (!valid_mid)
                            {
                                mid = p_dst_id;
                                p_val_phase = 1;
                            }
                        }
                        else
                        {
                            if (R->num_nodes > 2)
                            {
                                while (mid == node || mid == p_dst_id)
                                    mid = (int)(fast_rand(&seed) % R->num_nodes);
                            }
                            else
                            {
                                mid = p_dst_id;
                                p_val_phase = 1;
                            }
                        }

                        if (p_val_phase == 0)
                        {
                            p_val_mid = mid;
                            inject_route_dst = p_val_mid;
                        }
                    }

                    if (route_lut_mode == 4 && p_dst_id != node)
                    {
                        if (path_diversity_mode == 2 && p_val_phase == 0 && p_val_mid != -1 && p_val_mid != p_dst_id)
                        {
                            unsigned char l1 = R->sr_len[node * R->num_nodes + p_val_mid];
                            unsigned char l2 = R->sr_len[p_val_mid * R->num_nodes + p_dst_id];
                            if (l1 >= 2 && l2 >= 2 && (int)l1 + (int)l2 - 1 <= (ROUTE_SIM_ANYTOPO_DEPTH + 1))
                            {
                                int base1 = (node * R->num_nodes + p_val_mid) * (ROUTE_SIM_ANYTOPO_DEPTH + 1);
                                int base2 = (p_val_mid * R->num_nodes + p_dst_id) * (ROUTE_SIM_ANYTOPO_DEPTH + 1);
                                sr_len_local = (unsigned char)(l1 + l2 - 1);
                                for (int t = 0; t < l1; ++t) sr_nodes_local[t] = R->sr_nodes[base1 + t];
                                for (int t = 1; t < l2; ++t) sr_nodes_local[l1 - 1 + t] = R->sr_nodes[base2 + t];
                            }
                            else
                            {
                                p_val_mid = p_dst_id;
                                p_val_phase = 1;
                            }
                        }
                        if (sr_len_local == 0)
                        {
                            unsigned char l = R->sr_len[node * R->num_nodes + p_dst_id];
                            if (l >= 2)
                            {
                                int base = (node * R->num_nodes + p_dst_id) * (ROUTE_SIM_ANYTOPO_DEPTH + 1);
                                sr_len_local = l;
                                for (int t = 0; t < l; ++t) sr_nodes_local[t] = R->sr_nodes[base + t];
                            }
                        }
                    }

                    if (route_lut_mode == 4 && sr_len_local >= 2)
                    {
                        int outp = find_port_to_neighbor_id(R, r, sr_nodes_local[1]);
                        dst_port = (outp >= 0) ? (unsigned char)outp : 255;
                    }
                    else
                    {
                        dst_port = r->lut[dst_vc][R->degree][inject_route_dst];
                    }

                    if (p_dst_id == node && r->local_o.dst_id == -1 && !((path_diversity_mode == 2 || path_diversity_mode == 3) && p_val_phase == 0))
                    {
                        r->local_o.src_id = p_src_id;
                        r->local_o.dst_id = p_dst_id;
                        r->local_o.val_mid = p_val_mid;
                        r->local_o.val_phase = p_val_phase;
                        r->local_o.sr_len = sr_len_local;
                        r->local_o.sr_pos = 0;
                        memcpy(r->local_o.sr_nodes, sr_nodes_local, sizeof(r->local_o.sr_nodes));

                        r->local_channel[r->local_start].num--;
                        if (r->local_channel[r->local_start].num == 0)
                            r->local_start++;
                        l_busy = 1;
                        r->recv_num++;
                        r->total_time += 1;
                        if (1 > r->max_time) r->max_time = 1;
                        assert(r->total_time < (1ULL << 62));
                    }
                    else if (dst_port < R->degree && p_dst_id != node && r->channels[dst_port][dst_vc].buffer_o.dst_id == -1 && r->channels[dst_port][dst_vc].credit > 0)
                    {
                        Packet *out = &r->channels[dst_port][dst_vc].buffer_o;
                        out->src_id = p_src_id;
                        out->dst_id = p_dst_id;
                        out->start_time = sim_time;
                        out->ttl = ROUTE_SIM_TTL;
                        out->val_mid = p_val_mid;
                        out->val_phase = p_val_phase;
                        if (route_lut_mode == 4)
                        {
                            out->sr_len = sr_len_local;
                            out->sr_pos = 1;
                            memcpy(out->sr_nodes, sr_nodes_local, sizeof(out->sr_nodes));
                        }
                        else
                        {
                            out->sr_len = 0;
                            out->sr_pos = 0;
                        }

                        r->channels[dst_port][dst_vc].credit -= 1;
                        r->local_channel[r->local_start].num--;
                        if (r->local_channel[r->local_start].num == 0)
                            r->local_start++;
                        l_busy = 1;
                        path_log[r->id * R->degree + dst_port] += 1;
                    }
                }
            }

            // --- router_trans part 1 ---
            #pragma omp for
            for (int node = 0; node < R->num_nodes; ++node)
            {
                r = &(R->rl[node]);
                for (int i = 0; i < R->degree; ++i)
                {
                    for (int j = 0; j < R->vc_num; ++j)
                    {
                        if (r->channels[i][j].connect == NULL) continue;
                        if (r->channels[i][j].connect->buffer_o.dst_id != -1)
                        {
                            assert(r->channels[i][j].size < ROUTE_SIM_ANYTOPO_DEPTH);
                            Packet *in = &r->channels[i][j].buffer_i[r->channels[i][j].end];
                            Packet *src = &r->channels[i][j].connect->buffer_o;
                            *in = *src;
                            r->channels[i][j].end = (r->channels[i][j].end + 1) % ROUTE_SIM_ANYTOPO_DEPTH;
                            r->channels[i][j].size++;
                            r->channels[i][j].connect->buffer_o.dst_id = -1;
                        }
                        r->channels[i][j].connect->credit = ROUTE_SIM_ANYTOPO_DEPTH - r->channels[i][j].size;
                    }
                }
                r->local_o.dst_id = -1;
            }

            // --- router_trans part 2 ---
            if (path_diversity_mode != -1 && load_balance_mode == 2)
            {
                #pragma omp for collapse(2)
                for (int node = 0; node < R->num_nodes; ++node)
                {
                    for (int i = 0; i < R->degree; ++i)
                    {
                        r = &(R->rl[node]);
                        Router *l1_node = r->neighbor[i];
                        if (l1_node == NULL) continue;
                        for (int k = 0; k < R->vc_num; ++k)
                        {
                            int l1_rate = 0;
                            r->non_local_congestion[k][i] = 0;
                            for (int j = 0; j < R->degree; ++j)
                            {
                                int p_map = port_map[l1_node->id * R->num_nodes + r->id];
                                if (p_map >= R->degree) continue;
                                if (((l1_node->direction_viable[k][p_map] >> j) & 0x1) == 1)
                                {
                                    Router *l2_node = l1_node->neighbor[j];
                                    if (l2_node == r || l2_node == NULL) continue;
                                    int l2_rate = 0;
                                    int l2_credit_temp = 0;
                                    for (int m = 0; m < R->degree; ++m)
                                    {
                                        int p_map2 = port_map[l2_node->id * R->num_nodes + l1_node->id];
                                        if (p_map2 >= R->degree) continue;
                                        if (((l2_node->direction_viable[k][p_map2] >> m) & 0x1) == 1)
                                        {
                                            Router *l3_node = l2_node->neighbor[m];
                                            if (l3_node == r || l3_node == l1_node || l3_node == NULL) continue;
                                            l2_credit_temp += l2_node->channels[m][k].credit;
                                            l2_rate++;
                                        }
                                    }
                                    if (l1_node->channels[j][k].credit != 0)
                                    {
                                        r->non_local_congestion[k][i] += l2_credit_temp;
                                        r->non_local_congestion[k][i] += l1_node->channels[j][k].credit * l2_rate;
                                        l1_rate += l2_rate;
                                    }
                                }
                            }
                            if (r->channels[i][k].credit == 0)
                                r->non_local_congestion[k][i] = 0;
                            else
                                r->non_local_congestion[k][i] += r->channels[i][k].credit * l1_rate;
                        }
                    }
                }
            }

            #pragma omp single
            {
                if (l_empty == 0) router_empty = 0;
                if (l_busy == 1) router_busy = 1;

                if (router_busy == 0)
                {
                    if (router_empty == 1)
                    {
                        printf("Total cycle : %u\n", sim_time);
                        if (sim_time > 0)
                            printf("Throughput : %lf packets/cycle\n", (double)total_packets / (double)sim_time);
                        printf("End ...\n");
                        stop_flag = 1;
                    }
                    else if (dead_lock_cnt > 10)
                    {
                        char dumpname[256];
                        time_t t = time(NULL);
                        snprintf(dumpname, sizeof(dumpname), "results/deadlock_dump_%ld.txt", (long)t);
                        FILE *df = fopen(dumpname, "w");
                        if (df != NULL)
                        {
                            fprintf(df, "Deadlock detected at sim_time=%u\n", sim_time);
                            fprintf(df, "Params: path_diversity=%d load_balance=%d route_lut_mode=%d ppp=%d packets_num=%d\n",
                                    path_diversity_mode, load_balance_mode, route_lut_mode, ppp, packets_num);
                            for (int nid = 0; nid < R->num_nodes; ++nid)
                            {
                                Router *rr = &R->rl[nid];
                                fprintf(df, "\nNode %d: local_start=%d local_size=%d recv_num=%d total_time=%llu\n",
                                        nid, rr->local_start, rr->local_size, rr->recv_num, (unsigned long long)rr->total_time);
                                if (rr->local_channel != NULL)
                                {
                                    fprintf(df, "  local_channel:\n");
                                    for (int li = rr->local_start; li < rr->local_size; ++li)
                                        fprintf(df, "    dst=%d num=%d\n", rr->local_channel[li].dst_id, rr->local_channel[li].num);
                                }
                                for (int p = 0; p < R->degree; ++p)
                                {
                                    fprintf(df, "  port %d:\n", p);
                                    for (int v = 0; v < R->vc_num; ++v)
                                    {
                                        Channel *c = &rr->channels[p][v];
                                        fprintf(df, "    VC %d: size=%u start=%u end=%u credit=%u buffer_o.dst=%d\n",
                                                v, (unsigned)c->size, (unsigned)c->start, (unsigned)c->end, (unsigned)c->credit, c->buffer_o.dst_id);
                                        for (unsigned int q = 0; q < c->size; ++q)
                                        {
                                            int bi = (int)((c->start + q) % ROUTE_SIM_ANYTOPO_DEPTH);
                                            Packet *pp = &c->buffer_i[bi];
                                            int sr_cur = -1;
                                            int sr_next = -1;
                                            int sr_mismatch = 0;
                                            if (pp->sr_len > 0 && pp->sr_pos < pp->sr_len)
                                            {
                                                sr_cur = pp->sr_nodes[pp->sr_pos];
                                                if (pp->sr_pos + 1 < pp->sr_len)
                                                    sr_next = pp->sr_nodes[pp->sr_pos + 1];
                                                sr_mismatch = (sr_cur != nid);
                                            }
                                            fprintf(df, "      buf_i[%d]: src=%d dst=%d start_time=%u ttl=%d val_mid=%d val_phase=%u sr_len=%u sr_pos=%u sr=[%d,%d,%d,%d] sr_cur=%d sr_next=%d mismatch=%d\n",
                                                    bi, pp->src_id, pp->dst_id, pp->start_time, pp->ttl, pp->val_mid, (unsigned)pp->val_phase,
                                                    (unsigned)pp->sr_len, (unsigned)pp->sr_pos,
                                                    pp->sr_nodes[0], pp->sr_nodes[1], pp->sr_nodes[2], pp->sr_nodes[3],
                                                    sr_cur, sr_next, sr_mismatch);
                                        }
                                    }
                                }
                            }
                            fclose(df);
                            printf("Deadlock dump written to %s\n", dumpname);
                        }
                        else
                        {
                            printf("Failed to open deadlock dump file %s for writing\n", dumpname);
                        }
                        printf("Total cycle : %u\n", sim_time);
                        printf("****** ERROR ! Dead Lock ...\n");
                        stop_flag = 1;
                    }
                    else
                    {
                        dead_lock_cnt += 1;
                    }
                }
                else
                {
                    dead_lock_cnt = 0;
                }

                if (!stop_flag)
                {
                    sim_time += 1;
                    router_empty = 1;
                    router_busy = 0;
                }
            }
        }
    }

    *sim_time_ptr = sim_time;
}

void path_log_initial(const GraphAdjList *G, unsigned int **path_log)
{
    (*path_log) = (unsigned int *)calloc(G->num_nodes * G->degree, sizeof(unsigned int));
    assert((*path_log) != NULL);
}

void path_log_delete(const GraphAdjList *G, unsigned int *path_log)
{
    free(path_log);
    path_log = NULL;
}

void route_sim_anytopo(char *file_name, int traffic_mode, int route_lut_mode, unsigned int seed, int ppp, int packets_num, int root_select, int path_diversity_mode, int load_balance_mode, char *path_log_file_name, char *traffic_name, int traffic_num)
{
    printf("\n--------------------- Start of Rounte_Sim_Anytopo -------------------------\n");
    struct timeval time_start, time_end;

    // print parameters
    printf("**** Graph name is : %s\n", file_name);
    printf("**** Traffic Mode is : %d\n", traffic_mode);                                 // -1:read from file; 0:uniform random; 1:all-to-all; 2:hotspot
    printf("     seed = %u; ppp = %d/10000; packet_num = %d\n", seed, ppp, packets_num); // 0-uniform random
    if (traffic_mode == -1)
        printf("     traffic LUT file name is: %s\n", traffic_name);
    else
        printf("     traffic LUT file name is: (n/a)\n");

    int hotspot_count = 1;
    int hot_rate_percent = 20;
    int hotspot_base = 0;
    {
        const char *e;
        e = getenv("HOTSPOT_COUNT");
        if (e != NULL)
            hotspot_count = atoi(e);
        e = getenv("HOT_RATE_PERCENT");
        if (e != NULL)
            hot_rate_percent = atoi(e);
        e = getenv("HOTSPOT_BASE");
        if (e != NULL)
            hotspot_base = atoi(e);
    }
    if (hotspot_count < 1)
        hotspot_count = 1;
    if (hot_rate_percent < 0)
        hot_rate_percent = 0;
    if (hot_rate_percent > 100)
        hot_rate_percent = 100;
    if (hotspot_base < 0)
        hotspot_base = 0;

    if (traffic_mode == 2)
    {
        printf("     hotspot config: hot_rate=%d%%, hotspot_count=%d, hotspot_base=%d\n", hot_rate_percent, hotspot_count, hotspot_base);
        printf("     hotspot meaning: %d%% traffic to fixed hotspot nodes, %d%% uniform random\n", hot_rate_percent, 100 - hot_rate_percent);
    }
    printf("**** Route Lut Mode is : %d\n", route_lut_mode);           // 0-L-turn; 1-Tree-turn; 2-Octo-turn; 3-SlimFly MIN; 4-Weighted Constrained; 5-Up/Down
    printf("**** Root selection mode is : %d\n", root_select);         // 0-fix; 1-random; 2-optimal select
    printf("**** Path Diversity mode is : %d\n", path_diversity_mode); // -1-no diversity; 0-minimal-diversity; 1-non-minimal diversity
    printf("**** Load Balance mode is : %d\n", load_balance_mode);     // 0-equal load; 1-local congestion aware; 2-non-local congestion aware
    printf("**** Num of VC is : %d\n", ROUTE_SIM_ANYTOPO_VC_RUNTIME);
    printf("**** Num of LUT is : %d\n", ROUTE_SIM_ANYTOPO_VC_RUNTIME);
    printf("**** Depth of Channel is : %d\n", ROUTE_SIM_ANYTOPO_DEPTH);
    printf("**** Path-log file name is : %s\n", path_log_file_name);

    GraphAdjList *G = construct_graph_adjlist_from_file(file_name);
    // GraphAdjList *G = jellyfish_gen(1100, 6, 0);
    // GraphAdjList *G = torus_gen(20, 2);
    // print_graph_adjlist(G);
    check_graph_adjlist(G);
    RouterList *R = construct_router_list_from_graphadjlist(G);

    //
    unsigned char *port_map = (unsigned char *)calloc(G->num_nodes * G->num_nodes, sizeof(unsigned char));
    assert(port_map != NULL);

    // Traffic Mode
    int **traffic_table = NULL;
    unsigned long long total_packets = 0;
    if (traffic_mode == -1)
        traffic_table = traffic_from_file(traffic_name, traffic_num);
    else if (traffic_mode == 0)
        traffic_table = traffic_uniform_random(R, ppp, packets_num, seed);
    else if (traffic_mode == 1)
        traffic_table = traffic_all_to_all(R, packets_num);
    else if (traffic_mode == 2)
        traffic_table = traffic_hotspot(R, ppp, packets_num, seed, hot_rate_percent, hotspot_count, hotspot_base);
    else
        assert(0);

    for (int i = 0; i < R->num_nodes; ++i) {
        for (int j = 0; j < R->num_nodes; ++j) {
            total_packets += traffic_table[i][j];
        }
    }
    printf("Total Packets : %llu\n", total_packets);

    init_router_list_traffic(R, traffic_table);
    traffic_table_delete(traffic_table, R->num_nodes);
    traffic_table = NULL;

    // Route Lut Mode
    if (route_lut_mode == 0)
        init_router_list_lut_shortest_path(R, G, root_select, check_l_trun, port_map);
    else if (route_lut_mode == 1)
        init_router_list_lut_shortest_path(R, G, root_select, check_tree_trun, port_map);
    else if (route_lut_mode == 2)
        init_router_list_lut_shortest_path(R, G, root_select, check_octo_trun, port_map);
    else if (route_lut_mode == 3)
        init_router_list_lut_slimfly_min(R, G, port_map); /* SlimFly MIN (diameter-2 optimized) */
    else if (route_lut_mode == 4)
        init_router_list_lut_weighted_constrained(R, G, port_map); /* Weighted Constrained (Hop<=3) */
    else if (route_lut_mode == 5)
        init_router_list_lut_shortest_path(R, G, root_select, check_updown_turn, port_map); /* Up/Down (deadlock-free turn constraint) */
    else
        assert(0);

    // path log
    unsigned int *path_log = NULL;
    path_log_initial(G, &path_log);
    // start sim
    unsigned sim_time = 0;
    gettimeofday(&time_start, NULL);
    printf("Start ...\n");
    
    run_simulation_parallel(R, &sim_time, path_diversity_mode, load_balance_mode, port_map, path_log, ppp, packets_num, route_lut_mode, total_packets);

    gettimeofday(&time_end, NULL);
    double total_time = ((double)(time_end.tv_sec) + (double)(time_end.tv_usec) / 1000000.0) - ((double)(time_start.tv_sec) + (double)(time_start.tv_usec) / 1000000.0);
    printf("Using time = %f s\n", total_time);
    printf("**** Sim Done !\n");

    printf("Avg Packet Delivery Latency is : %lf\n", avg_packet_delivery_latency(R));
    printf("Max Packet Delivery Latency is : %u\n", max_packet_delivery_latency(R));

    write_path_log_to_file(G, path_log, path_log_file_name);

    // FREE
    free(port_map);
    path_log_delete(G, path_log);
}