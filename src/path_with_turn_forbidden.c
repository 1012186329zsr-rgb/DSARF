#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "path_with_turn_forbidden.h"
#include "floyd_warshall.h"
#include "omp.h"
#include "utils.h"

PathNode *new_path_node(int id, int depth, PathNode *father, int children_num)
{
    PathNode *node = (PathNode *)calloc(1, sizeof(PathNode));
    assert(node != NULL);
    node->id = id;
    node->depth = depth;
    node->father = father;
    node->children_num = children_num;
    return node;
}

void delete_path_line(PathNode *node)
{
    PathNode *father = NULL;
    while (node->children_num == 0)
    {
        father = node->father;
        free(node);
        if (father != NULL)
        {
            father->children_num--;
            node = father;
        }
        else
            break;
    }
}

// shortest path with turn forbidden
void trun_model_dijkstra(const GraphAdjList *G, const TreeCoordinate *tc, int root, unsigned int *dist, int **path, int (*turn_check)(int, int, int, int, int, int))
{
    assert(dist != NULL && path != NULL && tc != NULL && dist != NULL);
    memset(dist, INF, G->num_nodes * sizeof(int));
    int **visit = (int **)calloc(G->num_nodes, sizeof(int *)); // wheather one path is visited
    assert(visit != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        visit[i] = (int *)calloc(G->num_nodes, sizeof(int));
        assert(visit[i] != NULL);
        memset(path[i], -1, G->num_nodes * sizeof(int));
    }
    int queue_cap = G->num_nodes * G->degree;
    PathNode **queue = (PathNode **)calloc(queue_cap, sizeof(PathNode *));
    int queue_s = 0; // start
    int queue_e = 0; // end
    // push root in queue
    queue[queue_e++] = new_path_node(root, 0, NULL, 0); // root node
    dist[root] = 0;
    path[root][root] = root;
    // start BFS
    PathNode *this_node = 0;
    PathNode *pre_node = 0;
    // PathNode *dst_node = 0;
    int this_node_id = 0;
    int pre_node_id = 0;
    int dst_node_id = 0;
    int end = 0;
    while (queue_e > queue_s)
    {
        end = queue_e;
        while (queue_s < end)
        {
            this_node = queue[queue_s++];
            this_node_id = this_node->id;
            if (this_node->father != NULL)
            {
                pre_node = this_node->father;
                pre_node_id = pre_node->id;
            }
            else
                pre_node_id = this_node_id;
            for (int i = 0; i < G->degree; ++i)
            {
                dst_node_id = G->adj_list[this_node_id][i];
                if (dst_node_id != INF && visit[this_node_id][dst_node_id] == 0) // not visited path
                {
                    if ((*turn_check)(tc->x[pre_node_id], tc->y[pre_node_id], tc->x[this_node_id], tc->y[this_node_id], tc->x[dst_node_id], tc->y[dst_node_id]) != 0)
                    {
                        visit[this_node_id][dst_node_id] = 1;
                        if (queue_e >= queue_cap) {
                            /* grow queue when capacity exceeded */
                            int new_cap = queue_cap * 2;
                            PathNode **new_queue = (PathNode **)realloc(queue, new_cap * sizeof(PathNode *));
                            if (new_queue == NULL) {
                                /* allocation failed: abort to avoid undefined behavior */
                                fprintf(stderr, "Error: failed to expand queue in trun_model_dijkstra\n");
                                exit(EXIT_FAILURE);
                            }
                            /* zero new area to be safe */
                            memset(new_queue + queue_cap, 0, (new_cap - queue_cap) * sizeof(PathNode *));
                            queue = new_queue;
                            queue_cap = new_cap;
                        }
                        queue[queue_e++] = new_path_node(dst_node_id, this_node->depth + 1, this_node, 0);
                        this_node->children_num++;
                        if (dist[dst_node_id] == INF)
                        {
                            dist[dst_node_id] = this_node->depth + 1;
                            // record path
                            path[dst_node_id][dst_node_id] = this_node_id;
                            PathNode *leaf_node = this_node;
                            while (leaf_node->father != NULL)
                            {
                                path[dst_node_id][leaf_node->id] = leaf_node->father->id;
                                leaf_node = leaf_node->father;
                            }
                        }
                    }
                }
            }
            if (this_node->children_num == 0)
                delete_path_line(this_node);
        }
    }
    free(queue);
    for (int i = 0; i < G->num_nodes; ++i)
        free(visit[i]);
    free(visit);
}

void trun_model_dijkstra_parallel(const GraphAdjList *G, const TreeCoordinate *tc, unsigned int **dist, int ***path, int (*turn_check)(int, int, int, int, int, int))
{
    // omp_set_num_threads(PROCS_NUM);
#pragma omp parallel for
    for (int i = 0; i < G->num_nodes; ++i)
        trun_model_dijkstra(G, tc, i, dist[i], path[i], (*turn_check));
    floyd_check_dist(G, dist);
}

void turn_model_check_path(int num_nodes, int ***path, TreeCoordinate *tc, int (*turn_check)(int, int, int, int, int, int))
{
    // printf("------------------ turn check ---------------------\n");
    // omp_set_num_threads(PROCS_NUM);
#pragma omp parallel for
    // turn model check
    for (int i = 0; i < num_nodes; ++i) // src node
    {
        // printf("******* src %d  ********\n", i);
        // print_matrix(path[i], num_nodes, "123");
        int pre_node, this_node, post_node;
        for (int j = 0; j < num_nodes; ++j) // dst node
        {
            post_node = j;
            this_node = path[i][j][post_node];
            while (this_node != i)
            {
                pre_node = path[i][j][this_node];
                assert((*turn_check)(tc->x[pre_node], tc->y[pre_node], tc->x[this_node], tc->y[this_node], tc->x[post_node], tc->y[post_node]) != 0);
                post_node = this_node;
                this_node = pre_node;
            }
        }
    }
}

void turn_model_initial_path_dist(const GraphAdjList *G, unsigned int ***dist, int ****path)
{
    (*dist) = (unsigned int **)calloc(G->num_nodes, sizeof(unsigned int *));
    (*path) = (int ***)calloc(G->num_nodes, sizeof(int **));
    assert(*dist != NULL && *path != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        (*dist)[i] = (unsigned int *)calloc(G->num_nodes, sizeof(unsigned int));
        (*path)[i] = (int **)calloc(G->num_nodes, sizeof(int *));
        assert((*dist)[i] != NULL && (*path)[i] != NULL);
        for (int j = 0; j < G->num_nodes; ++j)
        {
            (*path)[i][j] = (int *)calloc(G->num_nodes, sizeof(int));
            assert((*path)[i][j] != NULL);
        }
    }
}

void turn_model_delete_path_dist(const GraphAdjList *G, unsigned int **dist, int ***path)
{
    for (int i = 0; i < G->num_nodes; ++i)
    {
        for (int j = 0; j < G->num_nodes; ++j)
            free(path[i][j]);
        free(path[i]);
        free(dist[i]);
    }
    free(dist);
    free(path);
}

double turn_model_cost(const GraphAdjList *G, unsigned int **dist, int ***path, unsigned int *history_path_use_array, int update_en)
{
    // average dist
    unsigned long sum = 0;
    for (int i = 0; i < G->num_nodes; ++i)
        for (int j = 0; j < G->num_nodes; ++j)
            sum += dist[i][j];
    double average = (double)((long double)sum / (long double)(G->num_nodes * G->num_nodes));
    // traffic balance
    int **path_use = (int **)calloc(G->num_nodes, sizeof(int *));
    assert(path_use != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        path_use[i] = (int *)calloc(G->num_nodes, sizeof(int));
        assert(path_use[i] != NULL);
    }
    int pre_node, post_node;
    for (int i = 0; i < G->num_nodes; ++i)
    {
        for (int j = 0; j < G->num_nodes; ++j)
        {
            post_node = j;
            pre_node = path[i][j][post_node];
            while (pre_node != i)
            {
                path_use[pre_node][post_node] += 1;
                post_node = pre_node;
                pre_node = path[i][j][post_node];
            }
        }
    }
    unsigned int *path_use_array = NULL;
    if (update_en == 0)
    {
        path_use_array = (unsigned int *)calloc(G->num_nodes * G->degree, sizeof(unsigned int));
        memcpy(path_use_array, history_path_use_array, G->num_nodes * G->degree * sizeof(unsigned int));
    }
    else if (update_en == 1)
        path_use_array = history_path_use_array;
    else
        assert(0);
    int ptr = 0;
    for (int i = 0; i < G->num_nodes; ++i)
    {
        for (int j = 0; j < G->num_nodes; ++j)
        {
            if (path_use[i][j] > 0)
            {
                assert(ptr < G->num_nodes * G->degree);
                path_use_array[ptr++] += path_use[i][j];
            }
        }
    }
    double var = variance(G->num_nodes * G->degree, path_use_array);
    // free
    for (int i = 0; i < G->num_nodes; ++i)
        free(path_use[i]);
    free(path_use);
    if (update_en == 0)
        free(path_use_array);
    return 1000.0 * average + 1.0 * var;
}

int *root_selection(const GraphAdjList *G, int (*turn_check)(int, int, int, int, int, int), int root_num)
{
    TreeCoordinate *tc;
    int *best_root = (int *)calloc(root_num, sizeof(int));
    assert(best_root != NULL);
    unsigned int **dist = NULL;
    int ***path = NULL;
    turn_model_initial_path_dist(G, &dist, &path);
    unsigned int *history_path_use_array = (unsigned int *)calloc(G->num_nodes * G->degree, sizeof(unsigned int));
    for (int k = 0; k < root_num; ++k) // every root
    {
        double smallest_cost = 0;
        double this_cost = 0;
        for (int i = 0; i < G->num_nodes; ++i)
        {
            tc = gen_tree_coordinate(G, i, 1);
            trun_model_dijkstra_parallel(G, tc, dist, path, turn_check);
            // cost
            this_cost = turn_model_cost(G, dist, path, history_path_use_array, 0);
            if (this_cost < smallest_cost || i == 0)
            {
                smallest_cost = this_cost;
                best_root[k] = i;
            }
            delete_tc(tc);
        }
        // record path-use info
        tc = gen_tree_coordinate(G, best_root[k], 1);
        trun_model_dijkstra_parallel(G, tc, dist, path, turn_check);
        this_cost = turn_model_cost(G, dist, path, history_path_use_array, 1);
        delete_tc(tc);
    }
    turn_model_delete_path_dist(G, dist, path);
    free(history_path_use_array);
    return best_root;
}

// shortest path with turn forbidden
void trun_model_bfs(const GraphAdjList *G, const TreeCoordinate *tc, int root, unsigned int *dist, int **path, int (*turn_check)(int, int, int, int, int, int))
{
    assert(dist != NULL && path != NULL && tc != NULL && dist != NULL);
    memset(dist, INF, G->num_nodes * sizeof(int));
    int *visit = (int *)calloc(G->num_nodes, sizeof(int)); // wheather one node is visited ( all connected nodes are visited )
    assert(visit != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
        memset(path[i], -1, G->num_nodes * sizeof(int));
    PathNode **queue = (PathNode **)calloc(G->num_nodes * G->degree, sizeof(PathNode *));
    int queue_s = 0; // strat
    int queue_e = 0; // end
    int length = 0;
    int max_length = G->num_nodes * G->degree;
    // push root in queue
    queue[queue_e++] = new_path_node(root, 0, NULL, 0); // root node
    length++;
    dist[root] = 0;
    path[root][root] = root;
    // start BFS
    PathNode *this_node = NULL;
    PathNode *pre_node = NULL;
    // PathNode *dst_node = 0;
    int this_node_id = 0;
    int pre_node_id = 0;
    int dst_node_id = 0;
    int end = 0;
    int all_neighbor_dist = 0;
    while (queue_e != queue_s)
    {
        end = queue_e;
        while (queue_s != end)
        {
            this_node = queue[queue_s++];
            queue_s = queue_s % max_length;
            length--;
            this_node_id = this_node->id;
            if (this_node->father != NULL)
            {
                pre_node = this_node->father;
                pre_node_id = pre_node->id;
            }
            else
                pre_node_id = this_node_id;
            for (int i = 0; i < G->degree; ++i)
            {
                dst_node_id = G->adj_list[this_node_id][i];
                if (dst_node_id != INF && visit[dst_node_id] == 0) // not visited path
                {
                    if ((*turn_check)(tc->x[pre_node_id], tc->y[pre_node_id], tc->x[this_node_id], tc->y[this_node_id], tc->x[dst_node_id], tc->y[dst_node_id]) != 0)
                    {
                        // visit[dst_node_id] = 1;
                        queue[queue_e++] = new_path_node(dst_node_id, this_node->depth + 1, this_node, 0);
                        queue_e = queue_e % max_length;
                        length++;
                        assert(length < max_length);
                        this_node->children_num++;
                        if (dist[dst_node_id] == INF)
                        {
                            dist[dst_node_id] = this_node->depth + 1;
                            // record path
                            path[dst_node_id][dst_node_id] = this_node_id;
                            PathNode *leaf_node = this_node;
                            while (leaf_node->father != NULL)
                            {
                                path[dst_node_id][leaf_node->id] = leaf_node->father->id;
                                leaf_node = leaf_node->father;
                            }
                        }
                    }
                }
            }
            all_neighbor_dist = 0;
            for (int i = 0; i < G->degree; ++i)
            {
                if (G->adj_list[this_node_id][i] != INF && dist[G->adj_list[this_node_id][i]] == INF)
                    all_neighbor_dist = 1;
            }
            if (all_neighbor_dist == 0)
                visit[this_node_id] = 1;
            if (this_node->children_num == 0)
                delete_path_line(this_node);
        }
    }
    free(queue);
    free(visit);
}

void trun_model_bfs_parallel(const GraphAdjList *G, const TreeCoordinate *tc, unsigned int **dist, int ***path, int (*turn_check)(int, int, int, int, int, int))
{
    // omp_set_num_threads(PROCS_NUM);
#pragma omp parallel for
    for (int i = 0; i < G->num_nodes; ++i)
        trun_model_bfs(G, tc, i, dist[i], path[i], (*turn_check));
    floyd_check_dist(G, dist);
}

void turn_model_check_dist_new(const GraphAdjList *G, unsigned int ***dist)
{
    int false_path_num = 0;
    for (int d = 0; d < G->degree; ++d)
        for (int i = 0; i < G->num_nodes; ++i)
            for (int j = 0; j < G->num_nodes; ++j)
                if (dist[i][d][j] == INF)
                    false_path_num++;
    for (int d = G->degree; d < G->degree + 1; ++d)
        for (int i = 0; i < G->num_nodes; ++i)
            for (int j = 0; j < G->num_nodes; ++j)
                assert(dist[i][d][j] < INF);
    double false_dist_rate = (double)false_path_num / (double)(G->degree * G->num_nodes * G->num_nodes);
    printf("False dist rate is %lf\n", false_dist_rate);
}


// shortest path with turn forbidden (find every shortest path for every input channel)
void trun_model_bfs_new(const GraphAdjList *G, TreeCoordinate **tc, RouterList *R, int (*turn_check)(int, int, int, int, int, int))
{
    Router *router_list = R->rl;
    int vc_num = R->vc_num;
    // initial LUT and SELF_DST
    for (int i = 0; i < R->num_nodes; ++i)
    {
        for (int j = 0; j < vc_num; ++j)
        {
            router_list[i].lut[j] = (unsigned char **)calloc(R->degree + 1, sizeof(unsigned char *));
            router_list[i].self_dst[j] = (unsigned int **)calloc(R->degree + 1, sizeof(unsigned int *));
            assert(router_list[i].lut[j] != NULL && router_list[i].self_dst[j] != NULL);
            for (int k = 0; k < R->degree + 1; ++k)
            {
                router_list[i].lut[j][k] = (unsigned char *)calloc(R->num_nodes, sizeof(unsigned char));
                router_list[i].self_dst[j][k] = (unsigned int *)calloc(R->num_nodes, sizeof(unsigned int));
                assert(router_list[i].lut[j][k] != NULL && router_list[i].self_dst[j][k] != NULL);
                memset(router_list[i].lut[j][k], 255, R->num_nodes * sizeof(unsigned char));
                memset(router_list[i].self_dst[j][k], INF, R->num_nodes * sizeof(unsigned int));
            }
        }
    }
    // initial LUT and SELF_DST
    // initial dst
    unsigned char **dst = (unsigned char **)calloc(R->num_nodes, sizeof(unsigned char *));
    assert(dst != NULL);
    for (int i = 0; i < R->num_nodes; ++i)
    {
        dst[i] = (unsigned char *)calloc(R->num_nodes, sizeof(unsigned char));
        assert(dst[i] != NULL);
        memset(dst[i], 255, R->num_nodes * sizeof(unsigned char));
        for (unsigned char j = 0; j < G->degree; ++j)
        {
            if (G->adj_list[i][j] != INF)
            {
                dst[i][G->adj_list[i][j]] = j;
            }
        }
    }
    // start BFS
    // Optimization: Parallelize over VC loop using OpenMP
    // Each thread gets its own set of temporary arrays to avoid malloc/free overhead inside loops
    // omp_set_num_threads(PROCS_NUM);
#pragma omp parallel
    {
        // Thread-private temporary arrays
        // State is now (node, input_port_index). 
        // input_port_index range: 0 to G->degree-1 (from neighbor), G->degree (injection/root)
        int state_num = G->num_nodes * (G->degree + 1);
        int *visited_state = (int *)malloc(state_num * sizeof(int));
        int *parent_state = (int *)malloc(state_num * sizeof(int)); // stores encoded parent state
        int *depth_state = (int *)malloc(state_num * sizeof(int));
        int *q = (int *)malloc(state_num * sizeof(int)); // stores encoded state
        assert(visited_state != NULL && parent_state != NULL && depth_state != NULL && q != NULL);

        // Thread-private loop variables
        int queue_s, queue_e;
        int pre_node_id;
        int u, v;
        int u_in_port, v_in_port; // port index on u (connected to pre), port index on v (connected to u)

        #pragma omp for collapse(1) schedule(dynamic)
        for (int vc_idx = 0; vc_idx < vc_num; ++vc_idx) // for every VC
        {
            for (int root = 0; root < R->num_nodes; ++root) // for every root node
            {
                for (int input_idx = 0; input_idx < G->degree + 1; ++input_idx) // for every input index
                {
                    router_list[root].self_dst[vc_idx][input_idx][root] = 0;
                    router_list[root].lut[vc_idx][input_idx][root] = 254;
                    
                    // BFS with (node, in_port) state
                    // Initialize BFS state
                    memset(visited_state, 0, state_num * sizeof(int));
                    for (int i = 0; i < state_num; ++i) {
                        parent_state[i] = -1;
                        depth_state[i] = -1;
                    }
                    
                    queue_s = 0;
                    queue_e = 0;
                    
                    // initialize root
                    // root state is (root, input_idx)
                    int root_state = root * (G->degree + 1) + input_idx;
                    parent_state[root_state] = root_state; // self-loop for root parent
                    depth_state[root_state] = 0;
                    visited_state[root_state] = 1;
                    q[queue_e++] = root_state;

                    while (queue_s < queue_e)
                    {
                        int curr_state = q[queue_s++];
                        u = curr_state / (G->degree + 1);
                        u_in_port = curr_state % (G->degree + 1);
                        
                        if (u == root && u_in_port == input_idx)
                        {
                            if (input_idx == G->degree || G->adj_list[u][input_idx] == INF)
                                pre_node_id = u;
                            else
                                pre_node_id = G->adj_list[u][input_idx];
                        }
                        else
                        {
                            // pre_node is the neighbor connected to u_in_port
                            // if u_in_port == G->degree, it means injection (should only happen at root)
                            if (u_in_port == G->degree) pre_node_id = u;
                            else pre_node_id = G->adj_list[u][u_in_port];
                        }

                        for (int ei = 0; ei < G->degree; ++ei)
                        {
                            v = G->adj_list[u][ei];
                            if (v == INF)
                                continue;
                            
                            // Find v's input port (which port on v connects to u?)
                            // Since we don't have reverse map efficiently, we search. Degree is small.
                            v_in_port = -1;
                            for(int k=0; k<G->degree; ++k) {
                                if (G->adj_list[v][k] == u) {
                                    v_in_port = k;
                                    break;
                                }
                            }
                            // If graph is consistent, v_in_port should be found. 
                            // Unless u==v (self loop) or error.
                            if (v_in_port == -1) continue; 

                            int next_state = v * (G->degree + 1) + v_in_port;
                            if (visited_state[next_state])
                                continue;

                            // check turn constraint: pre_node -> u -> v
                            if ((*turn_check)(tc[vc_idx]->x[pre_node_id], tc[vc_idx]->y[pre_node_id], tc[vc_idx]->x[u], tc[vc_idx]->y[u], tc[vc_idx]->x[v], tc[vc_idx]->y[v]) == 0)
                                continue;
                            
                            visited_state[next_state] = 1;
                            parent_state[next_state] = curr_state;
                            depth_state[next_state] = depth_state[curr_state] + 1;
                            q[queue_e++] = next_state;
                        }
                    }

                    // Debug print
                    if (input_idx == G->degree) {
                        int reachable_nodes = 0;
                        for(int i=0; i<G->num_nodes; ++i) {
                            int node_reachable = 0;
                            for(int p=0; p<=G->degree; ++p) {
                                if (visited_state[i * (G->degree + 1) + p]) {
                                    node_reachable = 1;
                                    break;
                                }
                            }
                            reachable_nodes += node_reachable;
                        }
                        if (reachable_nodes < G->num_nodes) {
                             printf("ERROR: root=%d vc=%d input=INJ reachable=%d/%d\n", root, vc_idx, reachable_nodes, G->num_nodes);
                        }
                    }

                    // For every reachable dst, backtrack parent chain and fill LUT/self_dst
                    for (int dst_idx = 0; dst_idx < R->num_nodes; ++dst_idx)
                    {
                        if (dst_idx == root)
                            continue;
                        
                        // Find best state for dst (shortest depth)
                        int best_dst_state = -1;
                        int min_depth = 99999999;
                        
                        for (int p = 0; p < G->degree; ++p) { // only check physical ports, not injection
                            int st = dst_idx * (G->degree + 1) + p;
                            if (visited_state[st] && depth_state[st] < min_depth) {
                                min_depth = depth_state[st];
                                best_dst_state = st;
                            }
                        }

                        if (best_dst_state == -1)
                            continue; // unreachable

                        // build path from root -> ... -> dst_idx
                        int len = 0;
                        int cur_state = best_dst_state;
                        
                        // reuse q array to store path states
                        while (1)
                        {
                            q[len++] = cur_state;
                            int cur_node = cur_state / (G->degree + 1);
                            int cur_port = cur_state % (G->degree + 1);
                            
                            if (cur_node == root && cur_port == input_idx)
                                break;
                            
                            if (parent_state[cur_state] == cur_state) break; // safety break
                            cur_state = parent_state[cur_state];
                        }
                        
                        // q[0..len-1] is dst_state->...->root_state, reverse iterate
                        for (int idx = len - 1; idx >= 1; --idx)
                        {
                            int P_state = q[idx];     
                            int C_state = q[idx - 1]; 
                            
                            int P = P_state / (G->degree + 1);
                            int C = C_state / (G->degree + 1);
                            
                            int in_port;
                            if (P == root && (P_state % (G->degree + 1)) == input_idx)
                                in_port = input_idx;
                            else {
                                // in_port is the port on P that P_state represents
                                in_port = P_state % (G->degree + 1);
                            }

                            // Find out_port from P to C
                            int out_port = -1;
                            for(int k=0; k<G->degree; ++k) {
                                if (G->adj_list[P][k] == C) {
                                    out_port = k;
                                    break;
                                }
                            }
                            
                            unsigned int distance = (unsigned int)(idx - 0); // hops from P to dst (idx steps)
                            if (router_list[P].self_dst[vc_idx][in_port][dst_idx] == INF || distance < router_list[P].self_dst[vc_idx][in_port][dst_idx])
                            {
                                router_list[P].self_dst[vc_idx][in_port][dst_idx] = distance;
                                router_list[P].lut[vc_idx][in_port][dst_idx] = out_port;
                            }
                        }
                    }
                }
            }
        }
        
        // Free thread-private memory
        free(visited_state);
        free(parent_state);
        free(depth_state);
        free(q);
    }
    
    for (int i = 0; i < R->num_nodes; ++i)
    {
        free(dst[i]);
    }
    free(dst);
}
