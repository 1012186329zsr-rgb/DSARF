#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <string.h>
#include "route_sim_anytopo.h"
#include "duato_vc_layering.h"
#include <stdlib.h>
#include "utils.h"
#include "floyd_warshall.h"
#include "dfs.h"
#include "tree_turn.h"
#include "l_turn.h"
#include "octo_turn.h"
#include "dfsssp_anytopo.h"
#include "topo_gen.h"
#include "path_with_turn_forbidden.h"
#include <omp.h>

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

double avg_packet_delivery_latency(const RouterList *R)
{
    unsigned long long total_time = 0;
    int total_packet_num = 0;
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
        // int p_dst_id, p_src_id;
        // unsigned int p_start_time;
        // int p_ttl;
        // Router *r = NULL;
        // unsigned char dst_port;
        // int dst_vc;
        // int viable_output_num;
        // int viable_output_list[128];
        // unsigned int seed;
        int p_dst_id, p_src_id;
        unsigned int p_start_time;
        int p_ttl;
        int p_hop;  // Duato-style VC: 已走过的跳数
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
        for (int node = 0; node < R->num_nodes; ++node) // every node
        {
            r = &(R->rl[node]);
            for (int i = 0; i < R->degree; ++i) // every input direction's channel
            {
                for (int j = 0; j < R->vc_num; ++j) // every VC
                {
                    if (r->channels[i][j].size > 0) // channel's buffer_i not empty
                    {
                        l_empty = 0; // not all packets have arrived destination
                        p_src_id = r->channels[i][j].buffer_i[r->channels[i][j].start].src_id;
                        p_dst_id = r->channels[i][j].buffer_i[r->channels[i][j].start].dst_id;
                        p_start_time = r->channels[i][j].buffer_i[r->channels[i][j].start].start_time;
                        p_ttl = r->channels[i][j].buffer_i[r->channels[i][j].start].ttl;
                        // Path Diversity  &  Load Balance //
                        // determine the output port by "path_diversity_mode" & "load_balance_mode"
                        if (p_dst_id != node)
                        {
                            if (path_diversity_mode == -1) // **** no diversity ****
                                dst_port = r->lut[j][i][p_dst_id];
                            else if (path_diversity_mode == 0) //  ****minimal diversity: only minimal-path can be choosed ****
                            {
                                int shortest_dst = r->self_dst[j][i][p_dst_id];
                                if (load_balance_mode == 0) // equal load
                                {
                                    viable_output_num = 0;
                                    for (int output_idx = 0; output_idx < R->degree; ++output_idx)
                                    {
                                        if (r->neighbor[output_idx] == NULL) continue;
                                        int neighbor_port = port_map[r->neighbor[output_idx]->id * R->num_nodes + r->id];
                                        if (neighbor_port >= R->degree) continue;
                                        if (r->neighbor[output_idx]->self_dst[j][neighbor_port][p_dst_id] == shortest_dst - 1)
                                            viable_output_list[viable_output_num++] = output_idx;
                                    }
                                    dst_port = viable_output_list[fast_rand(&seed) % viable_output_num];
                                }
                                else if (load_balance_mode == 1) // local congestion aware
                                {
                                    int biggest_credit = 0;
                                    viable_output_num = 0;
                                    // find the output channels, whoes credit is the biggest
                                    for (int output_idx = 0; output_idx < R->degree; ++output_idx)
                                    {
                                        if (r->neighbor[output_idx] == NULL) continue;
                                        int neighbor_port = port_map[r->neighbor[output_idx]->id * R->num_nodes + r->id];
                                        if (neighbor_port >= R->degree) continue;
                                        if (r->neighbor[output_idx]->self_dst[j][neighbor_port][p_dst_id] == shortest_dst - 1)
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
                                    if (viable_output_num == 0)
                                        dst_port = r->lut[j][i][p_dst_id];
                                    else
                                        dst_port = viable_output_list[fast_rand(&seed) % viable_output_num];
                                }
                                else if (load_balance_mode == 2) // non-local congestion aware
                                {
                                    int biggest_credit = 0;
                                    viable_output_num = 0;
                                    // find the output channels, whoes credit is the biggest
                                    for (int output_idx = 0; output_idx < R->degree; ++output_idx)
                                    {
                                        if (r->neighbor[output_idx] == NULL) continue;
                                        int neighbor_port = port_map[r->neighbor[output_idx]->id * R->num_nodes + r->id];
                                        if (neighbor_port >= R->degree) continue;
                                        if (r->neighbor[output_idx]->self_dst[j][neighbor_port][p_dst_id] == shortest_dst - 1 && r->channels[output_idx][j].buffer_o.dst_id == -1) // only select free channel
                                        {
                                            // assert(r->non_local_congestion[j][output_idx] == r->channels[output_idx][j].credit);
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
                                    if (viable_output_num == 0)
                                        dst_port = r->lut[j][i][p_dst_id];
                                    else
                                        dst_port = viable_output_list[fast_rand(&seed) % viable_output_num];
                                }
                                else
                                    assert(0);
                            }
                            else // **** non-minimal diversity ****
                            {
                                int shortest_dst = r->self_dst[j][i][p_dst_id];
                                if (load_balance_mode == 0) // equal load
                                {
                                    assert(0);
                                }
                                else if (load_balance_mode == 1) // local congestion aware
                                {
                                    int non_minimal_biggest_credit = 0;
                                    int non_minimal_best_port = -1;
                                    int minimal_biggest_credit = 0; // minimal path
                                    viable_output_num = 0;
                                    // find the minimal path output channels, whoes credit is the biggest
                                    for (int output_idx = 0; output_idx < R->degree; ++output_idx)
                                    {
                                        if (r->neighbor[output_idx] == NULL) continue;
                                        int neighbor_port = port_map[r->neighbor[output_idx]->id * R->num_nodes + r->id];
                                        if (neighbor_port >= R->degree) continue;
                                        if (r->neighbor[output_idx]->self_dst[j][neighbor_port][p_dst_id] == shortest_dst - 1)
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
                                            if (r->neighbor[output_idx] == NULL) continue;
                                            int neighbor_port = port_map[r->neighbor[output_idx]->id * R->num_nodes + r->id];
                                            if (neighbor_port < R->degree && r->neighbor[output_idx]->self_dst[j][neighbor_port][p_dst_id] != INF)
                                            {
                                                if (non_minimal_biggest_credit < r->channels[output_idx][j].credit)
                                                {
                                                    non_minimal_biggest_credit = r->channels[output_idx][j].credit;
                                                    non_minimal_best_port = output_idx;
                                                }
                                                else if (non_minimal_biggest_credit == r->channels[output_idx][j].credit && non_minimal_biggest_credit != 0)
                                                {
                                                    if (fast_rand(&seed) % 2 == 0)
                                                    {
                                                        non_minimal_biggest_credit = r->channels[output_idx][j].credit;
                                                        non_minimal_best_port = output_idx;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    if (p_ttl > 0)
                                    {
                                        // if (non_minimal_biggest_credit > minimal_biggest_credit) // choose non-minimal path
                                        if (non_minimal_biggest_credit == ROUTE_SIM_ANYTOPO_DEPTH && minimal_biggest_credit == 0)
                                        {
                                            dst_port = non_minimal_best_port;
                                            p_ttl--;
                                        }
                                        else
                                        { // choose minimal path
                                            if (viable_output_num == 0)
                                                dst_port = r->lut[j][i][p_dst_id];
                                            else
                                                dst_port = viable_output_list[fast_rand(&seed) % viable_output_num];
                                        }
                                    }
                                    else
                                    { // choose minimal path
                                        if (viable_output_num == 0)
                                            dst_port = r->lut[j][i][p_dst_id];
                                        else
                                            dst_port = viable_output_list[fast_rand(&seed) % viable_output_num];
                                    }
                                }
                                else if (load_balance_mode == 2) // non-local congestion aware
                                {
                                    int non_minimal_biggest_credit = 0;
                                    int non_minimal_best_port = -1;
                                    int minimal_biggest_credit = 0; // minimal path
                                    viable_output_num = 0;
                                    // find the minimal path output channels, whoes credit is the biggest
                                    for (int output_idx = 0; output_idx < R->degree; ++output_idx)
                                    {
                                        if (r->neighbor[output_idx] == NULL) continue;
                                        int neighbor_port = port_map[r->neighbor[output_idx]->id * R->num_nodes + r->id];
                                        if (neighbor_port >= R->degree) continue;
                                        if (r->neighbor[output_idx]->self_dst[j][neighbor_port][p_dst_id] == shortest_dst - 1 && r->channels[output_idx][j].buffer_o.dst_id == -1) // only select free channel
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
                                            if (r->neighbor[output_idx] == NULL) continue;
                                            int neighbor_port = port_map[r->neighbor[output_idx]->id * R->num_nodes + r->id];
                                            if (neighbor_port < R->degree && r->neighbor[output_idx]->self_dst[j][neighbor_port][p_dst_id] != INF && r->channels[output_idx][j].buffer_o.dst_id == -1) // only select free channel
                                            {
                                                if (non_minimal_biggest_credit < r->non_local_congestion[j][output_idx])
                                                {
                                                    non_minimal_biggest_credit = r->non_local_congestion[j][output_idx];
                                                    non_minimal_best_port = output_idx;
                                                }
                                                else if (non_minimal_biggest_credit == r->non_local_congestion[j][output_idx] && non_minimal_biggest_credit != 0)
                                                {
                                                    if (fast_rand(&seed) % 2 == 0)
                                                    {
                                                        non_minimal_biggest_credit = r->non_local_congestion[j][output_idx];
                                                        non_minimal_best_port = output_idx;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    if (p_ttl > 0)
                                    {
                                        // if (non_minimal_biggest_credit >= (R->degree * R->degree * R->degree * ROUTE_SIM_ANYTOPO_DEPTH) && minimal_biggest_credit == 0)
                                        if (non_minimal_best_port != -1 && r->channels[non_minimal_best_port][j].credit == ROUTE_SIM_ANYTOPO_DEPTH && minimal_biggest_credit == 0)
                                        {
                                            dst_port = non_minimal_best_port;
                                            p_ttl--;
                                        }
                                        else
                                        { // choose minimal path
                                            if (viable_output_num == 0)
                                                dst_port = r->lut[j][i][p_dst_id];
                                            else
                                                dst_port = viable_output_list[fast_rand(&seed) % viable_output_num];
                                        }
                                    }
                                    else
                                    { // choose minimal path
                                        if (viable_output_num == 0)
                                            dst_port = r->lut[j][i][p_dst_id];
                                        else
                                            dst_port = viable_output_list[fast_rand(&seed) % viable_output_num];
                                    }
                                }
                                else
                                    assert(0);
                            }
                        }
                        // printf("---- packet in NODE %d, channel %d, DST is %d, dst_port %d \t", node, i, p_dst_id, dst_port);
                        if (p_dst_id == node && r->local_o.dst_id == -1) // move to local
                        {
                            // printf("\t ** moved !\n");
                            r->local_o.src_id = p_src_id;
                            r->local_o.dst_id = p_dst_id;
                            r->local_o.start_time = p_start_time;
                            r->local_o.ttl = p_ttl;
                            r->channels[i][j].size -= 1;
                            r->channels[i][j].start = (r->channels[i][j].start + 1) % ROUTE_SIM_ANYTOPO_DEPTH; // pop this packet
                            l_busy = 1;                                                                  // at least onr packet is moving
                            r->recv_num++;
                            // r->total_time += sim_time;
                            unsigned int latency = sim_time - p_start_time + 1;
                            r->total_time += latency;
                            if (latency > r->max_time) r->max_time = latency;
                            assert(r->total_time < (1ULL << 62)); // in case of overflow
                        }
                        else if (dst_port < R->degree && p_dst_id != node && r->channels[dst_port][j].buffer_o.dst_id == -1 && r->channels[dst_port][j].credit > 0) // move to dst_port
                        {
                            // printf("\t ** moved !\n");
                            r->channels[dst_port][j].buffer_o.src_id = p_src_id; // assign to dst_port's buffer_o
                            r->channels[dst_port][j].buffer_o.dst_id = p_dst_id; // assign to dst_port's buffer_o
                            r->channels[dst_port][j].buffer_o.start_time = p_start_time;
                            r->channels[dst_port][j].buffer_o.ttl = p_ttl;
                            r->channels[dst_port][j].credit -= 1;
                            r->channels[i][j].size -= 1;
                            r->channels[i][j].start = (r->channels[i][j].start + 1) % ROUTE_SIM_ANYTOPO_DEPTH; // pop this packet
                            l_busy = 1;                                                                  // at least one packet is moving
                            path_log[r->id * R->degree + dst_port] += 1;                                                    // record the number of delivered packets for every path
                        }
                        // else
                        //     printf("\n");
                    }
                }
            }
            if (r->local_start < r->local_size) // local channel
            {
                l_empty = 0;
                p_src_id = node;
                p_dst_id = r->local_channel[r->local_start].dst_id;
                if (route_lut_mode == 3) {
                    /* DFSSSP: find the specific VC assigned to this path */
                    dst_vc = -1;
                    for (int v = 0; v < R->vc_num; ++v) {
                        if (r->lut[v][R->degree][p_dst_id] != 255) {
                            dst_vc = v;
                            break;
                        }
                    }
                    if (dst_vc == -1) dst_vc = 0; /* No path found, default to 0 (will likely drop/wait) */
                } else {
                    dst_vc = fast_rand(&seed) % R->vc_num; // random send to each VC
                }
                dst_port = r->lut[dst_vc][R->degree][p_dst_id];
                if (p_dst_id == node && r->local_o.dst_id == -1) // move to local
                {
                    r->local_o.src_id = p_src_id;
                    r->local_o.dst_id = p_dst_id;
                    // r->local_o.start_time = sim_time;
                    // r->local_o.ttl = 20;  // TTL
                    r->local_channel[r->local_start].num--; // pop this packet
                    if (r->local_channel[r->local_start].num == 0)
                        r->local_start++;
                    l_busy = 1;
                    r->recv_num++;
                    // r->total_time += sim_time;
                    r->total_time += 1;
                    if (1 > r->max_time) r->max_time = 1;
                    assert(r->total_time < (1ULL << 62)); // in case of overflow
                }
                else if (dst_port < R->degree && p_dst_id != node && r->channels[dst_port][dst_vc].buffer_o.dst_id == -1 && r->channels[dst_port][dst_vc].credit > 0) // move to dst_port
                {
                    r->channels[dst_port][dst_vc].buffer_o.src_id = p_src_id; // assign to dst_port's buffer_o
                    r->channels[dst_port][dst_vc].buffer_o.dst_id = p_dst_id; // assign to dst_port's buffer_o
                    r->channels[dst_port][dst_vc].buffer_o.start_time = sim_time;
                    r->channels[dst_port][dst_vc].buffer_o.ttl = ROUTE_SIM_TTL; // TTL
                    r->channels[dst_port][dst_vc].credit -= 1;
                    r->local_channel[r->local_start].num--; // pop this packet
                    if (r->local_channel[r->local_start].num == 0)
                        r->local_start++;
                    l_busy = 1;
                    path_log[r->id * R->degree + dst_port] += 1; // record the number of delivered packets for every path
                }
            }
        }

        // --- router_trans part 1 ---
        #pragma omp for
        for (int node = 0; node < R->num_nodes; ++node) // every node
        {
            r = &(R->rl[node]);
            for (int i = 0; i < R->degree; ++i)
            {
                for (int j = 0; j < R->vc_num; ++j)
                {
                    if (r->channels[i][j].connect != NULL)
                    {
                        if (r->channels[i][j].connect->buffer_o.dst_id != -1) // move packet from buffer_o to buffer_i
                        {
                            assert(r->channels[i][j].size < ROUTE_SIM_ANYTOPO_DEPTH);
                            r->channels[i][j].buffer_i[r->channels[i][j].end].src_id = r->channels[i][j].connect->buffer_o.src_id;
                            r->channels[i][j].buffer_i[r->channels[i][j].end].dst_id = r->channels[i][j].connect->buffer_o.dst_id;
                            r->channels[i][j].buffer_i[r->channels[i][j].end].start_time = r->channels[i][j].connect->buffer_o.start_time;
                            r->channels[i][j].buffer_i[r->channels[i][j].end].ttl = r->channels[i][j].connect->buffer_o.ttl;
                            r->channels[i][j].end = (r->channels[i][j].end + 1) % ROUTE_SIM_ANYTOPO_DEPTH;
                            r->channels[i][j].size++;
                            r->channels[i][j].connect->buffer_o.dst_id = -1; // clear
                        }
                        r->channels[i][j].connect->credit = ROUTE_SIM_ANYTOPO_DEPTH - r->channels[i][j].size; // update credit
                    }
                }
            }
            r->local_o.dst_id = -1; // clear local
        }

        // --- router_trans part 2 ---
        if (path_diversity_mode != -1 && load_balance_mode == 2)
        {
            #pragma omp for collapse(2)
            for (int node = 0; node < R->num_nodes; ++node)
            {
                for (int i = 0; i < R->degree; ++i) // output port
                {
                    r = &(R->rl[node]);
                    // update non-local congestion infomation
                    Router *l1_node, *l2_node, *l3_node;
                    l1_node = r->neighbor[i]; // next node
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
                                l2_node = l1_node->neighbor[j];
                                if (l2_node == r || l2_node == NULL)
                                    continue;
                                int l2_rate = 0;
                                int l2_credit_temp = 0;
                                for (int m = 0; m < R->degree; ++m)
                                {
                                    int p_map2 = port_map[l2_node->id * R->num_nodes + l1_node->id];
                                    if (p_map2 >= R->degree) continue;
                                    if (((l2_node->direction_viable[k][p_map2] >> m) & 0x1) == 1)
                                    {
                                        l3_node = l2_node->neighbor[m];
                                        if (l3_node == r || l3_node == l1_node || l3_node == NULL)
                                            continue;
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
                        {
                            r->non_local_congestion[k][i] = 0;
                        }
                        else
                        {
                            r->non_local_congestion[k][i] += r->channels[i][k].credit * l1_rate;
                        }
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
                    if (sim_time > 0) {
                        printf("Throughput : %lf packets/cycle\n", (double)total_packets / (double)sim_time);
                    }
                    printf("End ...\n");
                    stop_flag = 1;
                }
                else if (dead_lock_cnt > 10)
                {
                    /* Dump internal router/channel state to a file for post-mortem */
                    {
                        char dumpname[256];
                        time_t t = time(NULL);
                        snprintf(dumpname, sizeof(dumpname), "results/cayley/deadlock_dump_%ld.txt", (long)t);
                        FILE *df = fopen(dumpname, "w");
                        if (df != NULL)
                        {
                            fprintf(df, "Deadlock detected at sim_time=%u\n", sim_time);
                            fprintf(df, "Params: path_diversity=%d load_balance=%d route_lut_mode=%d ppp=%d packets_num=%d\n", path_diversity_mode, load_balance_mode, route_lut_mode, ppp, packets_num);
                            for (int nid = 0; nid < R->num_nodes; ++nid)
                            {
                                Router *rr = &R->rl[nid];
                                fprintf(df, "\nNode %d: local_start=%d local_size=%d recv_num=%d total_time=%llu\n", nid, rr->local_start, rr->local_size, rr->recv_num, (unsigned long long)rr->total_time);
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
                                        fprintf(df, "    VC %d: size=%u start=%u end=%u credit=%u buffer_o.dst=%d\n", v, (unsigned)c->size, (unsigned)c->start, (unsigned)c->end, (unsigned)c->credit, c->buffer_o.dst_id);
                                        for (int bi = 0; bi < ROUTE_SIM_ANYTOPO_DEPTH; ++bi)
                                        {
                                            Packet *pp = &c->buffer_i[bi];
                                            if (pp->dst_id != -1)
                                                fprintf(df, "      buf_i[%d]: src=%d dst=%d start_time=%u ttl=%d\n", bi, pp->src_id, pp->dst_id, pp->start_time, pp->ttl);
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
                    }
                    printf("Total cycle : %u\n", sim_time);
                    printf("****** ERROR ! Dead Lock ...\n");
                    stop_flag = 1;
                }
                else
                    dead_lock_cnt += 1;
            }
            else
                dead_lock_cnt = 0;
            
            if (!stop_flag) {
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
    printf("**** Traffic Mode is : %d\n", traffic_mode);                                 // -1:read from file; 0-uniform random
    printf("     seed = %u; ppp = %d/10000; packet_num = %d\n", seed, ppp, packets_num); // 0-uniform random
    printf("     traffic LUT file name is: %s\n", traffic_name);
    printf("**** Route Lut Mode is : %d\n", route_lut_mode);           // 0-L-turn; 1-Tree-turn; 2-new-turn
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
        traffic_table = traffic_uniform_random(R, ppp, packets_num, 0);
    else
        assert(0);

    for (int i = 0; i < R->num_nodes; ++i) {
        for (int j = 0; j < R->num_nodes; ++j) {
            total_packets += traffic_table[i][j];
        }
    }
    printf("Total Packets : %llu\n", total_packets);

    init_router_list_traffic(R, traffic_table);

    // Route Lut Mode
    if (route_lut_mode == 0)
        init_router_list_lut_shortest_path(R, G, root_select, check_l_trun, port_map);
    else if (route_lut_mode == 1)
        init_router_list_lut_shortest_path(R, G, root_select, check_tree_trun, port_map);
    else if (route_lut_mode == 2)
        init_router_list_lut_shortest_path(R, G, root_select, check_octo_trun, port_map);
    else if (route_lut_mode == 3)
        /* DFSSSP-style deadlock-free oblivious routing on arbitrary topology */
        init_router_list_lut_dfsssp(R, G, port_map, ROUTE_SIM_ANYTOPO_VC_RUNTIME);
   
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