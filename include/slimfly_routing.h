#ifndef __SLIMFLY_ROUTING_H__
#define __SLIMFLY_ROUTING_H__ 1

#include "route_sim_anytopo.h"
#include "topology.h"

/*
 * SlimFly MIN routing table generator (diameter-2 optimized).
 *
 * It fills:
 *   - R->rl[u].lut[vc][in_port][dst] : output port index on router u
 *   - R->rl[u].self_dst[vc][in_port][dst] : hop distance estimate (0/1/2/...)
 *   - R->rl[u].direction_viable[vc][in_port] : bitmap of viable output ports (conservative "allow-all" by default)
 *
 * The function expects G->adj_list[u][p] to be the neighbor node-id on port p, or INF if port unused.
 * port_map is an NxN flattened array: port_map[u*N + v] = output port on u to reach neighbor v, or 255 if not directly connected.
 */
void init_router_list_lut_slimfly_min(RouterList *R, const GraphAdjList *G, unsigned char *port_map);

#endif /* __SLIMFLY_ROUTING_H__ */
