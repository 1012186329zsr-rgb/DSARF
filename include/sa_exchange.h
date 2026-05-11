#ifndef __SA_EXCHANGE_H__
#define __SA_EXCHANGE_H__ 1

#include "topology.h"

void sa_swdc_exchange(GraphAdjList *G, int ary, int cube, int rand_degrees, int *node, int *port, int *visit, int *stack, int dst_threshold);

void sa_swdc_exchange_local(GraphAdjList *G, int ary, int cube, int rand_degrees, int *node, int *port, int *visit, int *stack, int dst_threshold);

void sa_jellyfish_exchange(GraphAdjList *G, int *node, int *port, int *visit, int *stack, int dst_threshold);

void sa_jellyfish_exchange_local(GraphAdjList *G, int *node, int *port, int *visit, int *stack, int dst_threshold);

#endif