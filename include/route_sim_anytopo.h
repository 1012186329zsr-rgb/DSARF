#ifndef __ROUTE_SIM_ANYTOPO_H__
#define __ROUTE_SIM_ANYTOPO_H__ 1

#include "topology.h"

/* Optional routing turn model (Up/Down) */
#include "updown_turn.h"

#define ROUTE_SIM_ANYTOPO_DEPTH 3
#define ROUTE_SIM_ANYTOPO_VC 3
// runtime-selectable VC count (default from macro above)
extern int ROUTE_SIM_ANYTOPO_VC_RUNTIME;
#define ROUTE_SIM_TTL 20

typedef struct
{
    int dst_id;
    int num;
} Packets;

typedef struct
{
    int src_id;
    int dst_id;
    unsigned int start_time;
    int ttl;

    /* Valiant two-phase routing support (optional) */
    int val_mid;               /* intermediate node id */
    unsigned char val_phase;   /* 0: routing to val_mid, 1: routing to dst_id */

    /* Source routing support (used by route_lut_mode==4) */
    unsigned char sr_len;      /* number of valid nodes in sr_nodes (0 means disabled/invalid) */
    unsigned char sr_pos;      /* current node index in sr_nodes */
    int sr_nodes[ROUTE_SIM_ANYTOPO_DEPTH + 1]; /* node sequence, e.g., [s, ..., d], up to 4 nodes */
} Packet; // -1 for invalid packet

typedef struct channel Channel;
struct channel
{
    Packet buffer_i[ROUTE_SIM_ANYTOPO_DEPTH];
    Packet buffer_o;
    Channel *connect; // connect to another Router's Channel
    unsigned char size;
    unsigned char start;
    unsigned char end;
    unsigned char credit;
};

typedef struct router Router;
struct router
{
    int id;
    // local channel
    Packets *local_channel;
    int local_size;
    int local_start;
    Packet local_o;
    // other channels
    Channel **channels; // [degree][VC]
    // time statistic
    int recv_num;
    unsigned long long total_time;
    unsigned int max_time;
    // LUT (dynamic by VC): lut[vc][input_degree + 1][num_nodes(dst)]
    unsigned char ***lut; // [VC][input_degree + 1][num_nodes(dst)]
    // for each input channel, the viable output direction of output
    unsigned char **direction_viable; // [VC][input_degree]
    // the shortest distance to every node from this node
    unsigned int ***self_dst; // [VC][input_degree + 1][num_nodes(dst)]
    Router **neighbor;
    // non-local congestion infomation
    int **non_local_congestion; // [VC][degree]
};

typedef struct
{
    int num_nodes;
    int degree;
    int vc_num; // number of virtual channels in this RouterList
    Router *rl;

    /* Source-route tables for Weighted Constrained (Hop<=3).
       sr_len[s*num_nodes + d] gives node-count (2..4), 0 if invalid.
       sr_nodes[(s*num_nodes + d)*(ROUTE_SIM_ANYTOPO_DEPTH+1) + k] gives node id at index k.
    */
    unsigned char *sr_len;
    int *sr_nodes;
} RouterList;

RouterList *construct_router_list_from_graphadjlist(GraphAdjList *G);

void delete_router_list_traffic(RouterList *R);

void delete_router_list_lut(RouterList *R);

void delete_router_list(RouterList *R);

void init_router_list_traffic(RouterList *R, int **traffic_table);

int **traffic_uniform_random(const RouterList *R, int p, int packet_num, unsigned int seed);

void route_sim_anytopo(char *file_name, int traffic_mode, int route_lut_mode, unsigned int seed, int ppp, int packets_num, int root_select, int path_diversity_mode, int load_balance_mode, char *path_log_file_name, char *traffic_name, int traffic_num);

double avg_packet_delivery_latency(const RouterList *R);

unsigned int max_packet_delivery_latency(const RouterList *R);

void init_router_list_lut_weighted_constrained(RouterList *R, const GraphAdjList *G, unsigned char *port_map);

#endif