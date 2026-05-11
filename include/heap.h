#ifndef __HEAP_H__
#define __HEAP_H__ 1

typedef struct
{
    int idx;
    unsigned int dist;
} HeapNode;

typedef struct
{
    HeapNode *list;
    int size;
    int capacity;
} MinHeap;

int mh_full(MinHeap *H);

int mh_empty(MinHeap *H);

void mh_print(MinHeap *H);

MinHeap *mh_init(int capacity);

void mh_insert(MinHeap *H, int idx, unsigned int dist);

HeapNode *mh_pop(MinHeap *H);

#endif