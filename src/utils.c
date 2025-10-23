#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "floyd_warshall.h"
#include "dfs.h"
#include "cord.h"

GraphAdjList *construct_graph_adjlist_from_file(char *file_name)
{
    GraphAdjList *G = (GraphAdjList *)calloc(1, sizeof(GraphAdjList));
    FILE *f = fopen(file_name, "r");
    assert(f != NULL);

    int scanned = fscanf(f, "%d,%d", &G->num_nodes, &G->degree);
    /* header read; continue with robust parsing */
    if (scanned != 2) {
        fprintf(stderr, "ERROR: failed to read num_nodes,degree from %s (fscanf returned %d)\n", file_name, scanned);
        fclose(f);
        free(G);
        return NULL;
    }
    if (G->num_nodes <= 0 || G->degree <= 0) {
        fprintf(stderr, "ERROR: invalid graph header in %s: num_nodes=%d degree=%d\n", file_name, G->num_nodes, G->degree);
        fclose(f);
        free(G);
        return NULL;
    }
    printf("----- start construct Graph AdjList from file, num_nodes = %d, degree = %d -----\n", G->num_nodes, G->degree);

    // alloc Adj-List
    G->adj_list = (unsigned int **)calloc(G->num_nodes, sizeof(unsigned int *));
    assert(G->adj_list != NULL);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        G->adj_list[i] = (unsigned int *)calloc(G->degree, sizeof(unsigned int));
        assert(G->adj_list[i] != NULL);
    }

    // read from file using robust fscanf to parse integers and separators
    int dst_node;
    int sep;
    for (int i = 0; i < G->num_nodes; ++i)
    {
        for (int j = 0; j < G->degree; ++j)
        {
            if (fscanf(f, " %d", &dst_node) != 1)
            {
                fprintf(stderr, "ERROR: failed to read dst node at row %d col %d\n", i, j);
                fclose(f);
                free(G->adj_list);
                free(G);
                return NULL;
            }
            G->adj_list[i][j] = (unsigned int)dst_node;
            if (j < G->degree - 1)
            {
                sep = fgetc(f);
                if (sep != ',')
                {
                    fprintf(stderr, "ERROR: expected comma after value at row %d col %d but got '%c'(%d)\n", i, j, sep, sep);
                }
            }
            else
            {
                // consume newline or EOF
                sep = fgetc(f);
                if (sep != '\n' && sep != EOF)
                {
                    // tolerate optional carriage return
                    if (sep == '\r') {
                        int nxt = fgetc(f);
                        if (nxt != '\n' && nxt != EOF) ungetc(nxt, f);
                    } else {
                        ungetc(sep, f);
                    }
                }
            }
        }
    }
    fclose(f);
    f = NULL;
    return G;
}

void print_graph_adjlist(GraphAdjList *G)
{
    printf("----- Graph AdjList, num_nodes = %d, degree = %d -----\n", G->num_nodes, G->degree);
    printf("src\t\t: dst\n");
    for (int i = 0; i < G->num_nodes; ++i)
    {
        printf("%d\t\t: ", i);
        for (int j = 0; j < G->degree; ++j)
        {
            printf("%d, ", G->adj_list[i][j]);
        }
        printf("\n");
    }
}

void write_graph_adjlist_to_file(GraphAdjList *G, char *file_name)
{
    FILE *f = fopen(file_name, "w");
    assert(fprintf(f, "%d,%d\n", G->num_nodes, G->degree) >= 0);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        for (int j = 0; j < G->degree - 1; ++j)
            assert(fprintf(f, "%d,", G->adj_list[i][j]) >= 0);
        assert(fprintf(f, "%d\n", G->adj_list[i][G->degree - 1]) >= 0);
    }
    fclose(f);
    f = NULL;
}

void write_path_log_to_file(const GraphAdjList *G, unsigned int **path_log, char *file_name)
{
    FILE *f = fopen(file_name, "w");
    assert(fprintf(f, "%d,%d\n", G->num_nodes, G->degree) >= 0);
    for (int i = 0; i < G->num_nodes; ++i)
    {
        for (int j = 0; j < G->degree - 1; ++j)
            assert(fprintf(f, "%d,", path_log[i][j]) >= 0);
        assert(fprintf(f, "%d\n", path_log[i][G->degree - 1]) >= 0);
    }
    fclose(f);
    f = NULL;
}

void delete_graph(GraphAdjList *G)
{
    for (int i = 0; i < G->num_nodes; ++i)
    {
        free(G->adj_list[i]);
        G->adj_list[i] = NULL;
    }
    free(G->adj_list);
    G->adj_list = NULL;
    free(G);
    G = NULL;
}

void check_graph_adjlist(GraphAdjList *G)
{
    int exist;
    for (int i = 0; i < G->num_nodes; ++i)
    {
        for (int j = 0; j < G->degree; ++j)
        {
            if (G->adj_list[i][j] != INF)
            {
                assert(G->adj_list[i][j] != i); // one cannot connect to self
                for (int k = 0; k < j; ++k)
                    assert(G->adj_list[i][j] != G->adj_list[i][k]); // one pair can only have one connection link
                // one link must exist in both nodes
                exist = 0;
                for (int k = 0; k < G->degree; ++k)
                    exist |= (G->adj_list[G->adj_list[i][j]][k] == i) ? 1 : 0;
                assert(exist == 1);
            }
        }
    }
}

int dst_torus(int *cord1, int *cord2, int ary, int cube)
{
    int res = 0;
    int temp;
    int half = ary / 2;
    for (int i = 0; i < cube; ++i)
    {
        temp = abs(cord1[i] - cord2[i]);
        temp = (temp > half) ? ary - temp : temp;
        res += temp;
    }
    return res;
}

void check_graph_dst_ring(GraphAdjList *G, int dst_threshold)
{
    int dst;
    int half = G->num_nodes / 2;
    for (int i = 0; i < G->num_nodes; ++i)
    {
        for (int j = 0; j < G->degree; ++j)
        {
            if (G->adj_list[i][j] != INF)
            {
                dst = abs(G->adj_list[i][j] - i);
                dst = (dst > half) ? G->num_nodes - dst : dst;
                assert(dst <= dst_threshold);
            }
        }
    }
}

void check_graph_dst_torus(GraphAdjList *G, int ary, int cube, int dst_threshold)
{
    int *cord1 = (int *)calloc(cube, sizeof(int));
    int *cord2 = (int *)calloc(cube, sizeof(int));
    int *weight = (int *)calloc(cube, sizeof(int));
    assert(cord1 != NULL && weight != NULL && cord2 != NULL);
    calculate_weight(ary, cube, weight);
    int dst = 0;
    for (int i = 0; i < G->num_nodes; ++i)
    {
        for (int j = 0; j < G->degree; ++j)
        {
            if (G->adj_list[i][j] != INF)
            {
                cord_fold(i, weight, cube, cord1);
                cord_fold(G->adj_list[i][j], weight, cube, cord2);
                dst = dst_torus(cord1, cord2, ary, cube);
                assert(dst <= dst_threshold);
            }
        }
    }
    free(cord1);
    cord1 = NULL;
    free(cord2);
    cord2 = NULL;
    free(weight);
    weight = NULL;
}

double variance(int length, unsigned int *array)
{
    unsigned long sum = 0;
    for (int i = 0; i < length; ++i)
        sum += array[i];
    double average = (double)((long double)sum / (long double)length);
    double sum_var = 0;
    for (int i = 0; i < length; ++i)
        sum_var += ((double)array[i] - average) * ((double)array[i] - average);
    return sum_var / (double)(length - 1);
}

int check_graph_connectivity(GraphAdjList *G)
{
    int *visit = (int *)calloc(G->num_nodes, sizeof(int));
    int *stack = (int *)calloc(G->num_nodes, sizeof(int));
    assert(visit != NULL && stack != NULL);
    dfs(G, visit, stack);
    int res = 1;
    for (int i = 0; i < G->num_nodes; ++i)
        res &= visit[i];
    free(visit);
    visit = NULL;
    return res; // 1 - connected; 0 - not connected
}

void print_matrix(int **matrix, int num, char *name)
{
    printf("\n_________ %s __________\n", name);
    printf("*\t");
    for (int i = 0; i < num; ++i)
        printf("%d\t", i);
    printf("\n");
    for (int i = 0; i < num; ++i)
    {
        printf("%d\t", i);
        for (int j = 0; j < num; ++j)
        {
            printf("%d\t", matrix[i][j] == INF ? -1 : matrix[i][j]);
        }
        printf("\n");
    }
}

void transpose(int nums, int **matrix)
{
    int temp;
    for (int i = 0; i < nums; ++i)
    {
        for (int j = i + 1; j < nums; ++j)
        {
            temp = matrix[i][j];
            matrix[i][j] = matrix[j][i];
            matrix[j][i] = temp;
        }
    }
}