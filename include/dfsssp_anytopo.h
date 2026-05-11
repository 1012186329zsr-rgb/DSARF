#ifndef __DFSSSP_ANYTOPO_H__
#define __DFSSSP_ANYTOPO_H__ 1

#include "route_sim_anytopo.h"

/*
 * Build Deadlock-Free Oblivious Routing (DFSSSP-style) LUTs for an arbitrary topology.
 *
 * Parameters:
 *   R        - router list constructed from the same GraphAdjList G
 *   G        - adjacency-list representation of the topology
 *   port_map - array of size (G->num_nodes * G->num_nodes). For each node u and neighbor v,
 *              port_map[u * G->num_nodes + v] gives the local output port index on u that
 *              connects to v. For non-neighbors it should contain 255.
 *   max_vc   - maximum number of virtual channels that DFSSSP is allowed to use.
 *              If max_vc <= 0 or max_vc > R->vc_num, R->vc_num is used.
 *
 * After this call, the following fields in R are initialised:
 *   - rl[i].lut[vc][input_idx][dst]       : output port (0..degree-1), 254 for local delivery, 255 invalid
 *   - rl[i].self_dst[vc][input_idx][dst]  : hop distance from node i to dst along the selected path
 *   - rl[i].direction_viable[vc][input_port] : bitmap of allowed output ports for load-balancing.
 *
 * The resulting per-VC channel dependency graph is acyclic, hence routing is deadlock-free
 * when each VC is modelled as a separate set of buffers in the simulator.
 */
void init_router_list_lut_dfsssp(RouterList *R,
                                 const GraphAdjList *G,
                                 unsigned char *port_map,
                                 int max_vc);

#endif /* __DFSSSP_ANYTOPO_H__ */
