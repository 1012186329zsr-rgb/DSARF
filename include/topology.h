#ifndef __TOPOLOGY_H__
#define __TOPOLOGY_H__ 1

// #define DEBUG

#ifdef DEBUG
#define debug(format, ...) printf(format, ##__VA_ARGS__)
#else
#define debug(format, ...)
#endif

#ifndef PROCS_NUM
#define PROCS_NUM 40
#endif

typedef struct
{
    int num_nodes;
    int degree;
    unsigned int **adj_list; // 1 - connected; 0 - not connected
} GraphAdjList;

typedef struct
{
    int num_nodes;
    int degree;
    unsigned int **matrix; // 1 - connected; 0 - not connected
} GraphMatrix;

#endif