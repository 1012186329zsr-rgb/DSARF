#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "slimfly_routing.h"
#include "floyd_warshall.h"  /* for INF */

/* Build port_map[u*N + v] from adjacency list. */
static void build_port_map(const GraphAdjList *G, unsigned char *port_map)
{
    int N = G->num_nodes;
    for (int u = 0; u < N; ++u)
    {
        memset(port_map + u * N, 255, N * sizeof(unsigned char));
        for (unsigned char p = 0; p < G->degree; ++p)
        {
            unsigned int v = G->adj_list[u][p];
            if (v != INF && v < (unsigned)N)
                port_map[u * N + (int)v] = p;
        }
    }
}

/* Conservative default: for each input port, allow any other output port that exists. */
static void fill_direction_viable_allow_all(RouterList *R, const GraphAdjList *G)
{
    for (int u = 0; u < R->num_nodes; ++u)
    {
        for (int vc = 0; vc < R->vc_num; ++vc)
        {
            if (R->rl[u].direction_viable[vc] == NULL)
            {
                R->rl[u].direction_viable[vc] = (unsigned char *)calloc(R->degree, sizeof(unsigned char));
                assert(R->rl[u].direction_viable[vc] != NULL);
            }
            for (int in_p = 0; in_p < R->degree; ++in_p)
            {
                unsigned char mask = 0;
                if (G->adj_list[u][in_p] == INF)
                {
                    R->rl[u].direction_viable[vc][in_p] = 0;
                    continue;
                }
                for (int out_p = 0; out_p < R->degree; ++out_p)
                {
                    if (out_p == in_p) continue;
                    if (G->adj_list[u][out_p] == INF) continue;
                    /* store as bitmap in lower bits (degree is expected <= 8 in this encoding) */
                    mask |= (unsigned char)(1u << out_p);
                }
                R->rl[u].direction_viable[vc][in_p] = mask;
            }
        }
    }
}

/* BFS fallback: compute next-hop port from src to dst, ignoring turn restrictions. */
static unsigned char bfs_next_port(const GraphAdjList *G, const unsigned char *port_map, int src, int dst)
{
    if (src == dst) return 254;

    int N = G->num_nodes;
    int *prev = (int *)malloc(N * sizeof(int));
    int *q = (int *)malloc(N * sizeof(int));
    unsigned char *vis = (unsigned char *)calloc(N, sizeof(unsigned char));
    assert(prev && q && vis);

    for (int i = 0; i < N; ++i) prev[i] = -1;
    int qs = 0, qe = 0;
    q[qe++] = src;
    vis[src] = 1;

    while (qs < qe)
    {
        int u = q[qs++];
        for (int p = 0; p < G->degree; ++p)
        {
            unsigned int v = G->adj_list[u][p];
            if (v == INF) continue;
            if (v >= (unsigned)N) continue;
            if (!vis[v])
            {
                vis[v] = 1;
                prev[v] = u;
                if ((int)v == dst)
                {
                    qs = qe; /* break outer */
                    break;
                }
                q[qe++] = (int)v;
            }
        }
    }

    unsigned char out = 255;
    if (vis[dst])
    {
        /* walk back from dst to find first hop */
        int cur = dst;
        int parent = prev[cur];
        while (parent != -1 && parent != src)
        {
            cur = parent;
            parent = prev[cur];
        }
        if (parent == src)
            out = port_map[src * N + cur];
    }

    free(prev);
    free(q);
    free(vis);
    return out;
}

void init_router_list_lut_slimfly_min(RouterList *R, const GraphAdjList *G, unsigned char *port_map)
{
    assert(R != NULL && G != NULL && port_map != NULL);
    assert(R->num_nodes == G->num_nodes);
    assert(R->degree == G->degree);

    int N = R->num_nodes;

    /* (Re)build port_map from G. */
    build_port_map(G, port_map);

    /* reset existing LUT */
    delete_router_list_lut(R);

    /* allocate LUT / SELF_DST for all routers + all VCs */
    for (int u = 0; u < N; ++u)
    {
        for (int vc = 0; vc < R->vc_num; ++vc)
        {
            R->rl[u].lut[vc] = (unsigned char **)calloc(R->degree + 1, sizeof(unsigned char *));
            R->rl[u].self_dst[vc] = (unsigned int **)calloc(R->degree + 1, sizeof(unsigned int *));
            assert(R->rl[u].lut[vc] && R->rl[u].self_dst[vc]);

            for (int in_p = 0; in_p < R->degree + 1; ++in_p)
            {
                R->rl[u].lut[vc][in_p] = (unsigned char *)malloc(N * sizeof(unsigned char));
                R->rl[u].self_dst[vc][in_p] = (unsigned int *)malloc(N * sizeof(unsigned int));
                assert(R->rl[u].lut[vc][in_p] && R->rl[u].self_dst[vc][in_p]);

                /* init invalid */
                memset(R->rl[u].lut[vc][in_p], 255, N * sizeof(unsigned char));
                for (int d = 0; d < N; ++d)
                    R->rl[u].self_dst[vc][in_p][d] = INF;
            }
        }
    }

    /* Build neighbor sets as bitmarks for diameter-2 intersection test (O(N*deg^2)). */
    /* For each u, mark its neighbors */
    unsigned char *mark = (unsigned char *)calloc(N, sizeof(unsigned char));
    assert(mark != NULL);

    for (int u = 0; u < N; ++u)
    {
        memset(mark, 0, N * sizeof(unsigned char));
        for (int p = 0; p < R->degree; ++p)
        {
            unsigned int v = G->adj_list[u][p];
            if (v != INF && v < (unsigned)N)
                mark[v] = 1;
        }

        for (int d = 0; d < N; ++d)
        {
            if (d == u)
            {
                for (int vc = 0; vc < R->vc_num; ++vc)
                {
                    for (int in_p = 0; in_p < R->degree + 1; ++in_p)
                    {
                        R->rl[u].lut[vc][in_p][d] = 254;   /* self */
                        R->rl[u].self_dst[vc][in_p][d] = 0;
                    }
                }
                continue;
            }

            unsigned char next_port = 255;
            unsigned int dist = INF;

            /* 1-hop? */
            unsigned char p1 = port_map[u * N + d];
            if (p1 != 255)
            {
                next_port = p1;
                dist = 1;
            }
            else
            {
                /* 2-hop? Find m in N(u) ∩ N(d). */
                int mid = -1;
                for (int p = 0; p < R->degree; ++p)
                {
                    unsigned int m = G->adj_list[d][p];
                    if (m == INF || m >= (unsigned)N) continue;
                    if (mark[m])
                    {
                        mid = (int)m;
                        break;
                    }
                }
                if (mid != -1)
                {
                    next_port = port_map[u * N + mid];
                    dist = 2;
                }
                else
                {
                    /* Fallback BFS */
                    next_port = bfs_next_port(G, port_map, u, d);
                    if (next_port != 255)
                    {
                        /* compute distance roughly by one BFS pass (cheap) */
                        /* simple upper-bound: we can re-run BFS to get exact dist; keep INF if unknown */
                        dist = INF;
                    }
                }
            }

            for (int vc = 0; vc < R->vc_num; ++vc)
            {
                for (int in_p = 0; in_p < R->degree + 1; ++in_p)
                {
                    R->rl[u].lut[vc][in_p][d] = next_port;
                    R->rl[u].self_dst[vc][in_p][d] = dist;
                }
            }
        }
    }

    free(mark);

    /* direction_viable: make sure it is allocated for algorithms that use it */
    fill_direction_viable_allow_all(R, G);
}
