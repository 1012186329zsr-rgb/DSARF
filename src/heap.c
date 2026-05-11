#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "heap.h"
#include "floyd_warshall.h"

int mh_full(MinHeap *H)
{
    return (H->size == H->capacity) ? 1 : 0;
}

int mh_empty(MinHeap *H)
{
    return (H->size == 0) ? 1 : 0;
}

void mh_print(MinHeap *H)
{
    printf("--------- Min Heap ----------\n");
    for (int i = 1; i <= H->size; ++i)
    {
        printf("(%u, %d) ", H->list[i].idx, H->list[i].dist);
        if ((i - 1) % 10 == 0 && i != 1)
            printf("\n");
    }
    printf("\n");
}

MinHeap *mh_init(int capacity)
{
    MinHeap *H = (MinHeap *)calloc(1, sizeof(MinHeap));
    assert(H != NULL);
    H->list = (HeapNode *)calloc((capacity + 1), sizeof(HeapNode));
    H->size = 0;
    H->capacity = capacity;
    return H;
}

void mh_insert(MinHeap *H, int idx, unsigned int dist)
{
    // TODO merge same node
    assert(!mh_full(H));
    int i;
    for (i = ++(H->size); dist < H->list[i / 2].dist; i /= 2)
        H->list[i] = H->list[i / 2];
    H->list[i].idx = idx;
    H->list[i].dist = dist;
}

HeapNode *mh_pop(MinHeap *H)
{
    HeapNode *last_node;
    HeapNode *min_node = (HeapNode *)calloc(1, sizeof(HeapNode));
    assert(min_node != NULL);
    int child, parent;
    assert(!mh_empty(H));

    min_node->idx = H->list[1].idx;
    min_node->dist = H->list[1].dist;
    last_node = &H->list[H->size--];
    for (parent = 1; 2 * parent <= H->size; parent = child)
    {
        child = 2 * parent;                                                    /*默认左结点的元素值更小*/
        if (child != H->size && H->list[child + 1].dist < H->list[child].dist) /*若右节点的元素值更小，则调整child*/
            child++;
        if (last_node->dist < H->list[child].dist)
            break;
        else
        {
            H->list[parent].idx = H->list[child].idx;
            H->list[parent].dist = H->list[child].dist;
        }
    }
    H->list[parent].idx = last_node->idx;
    H->list[parent].dist = last_node->dist;
    return min_node;
}