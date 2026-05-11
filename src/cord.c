#include <stdlib.h>
#include <assert.h>

void calculate_weight(int ary, int cube, int *weight)
{
    assert(cube > 0 && ary > 0);
    // int *weight = (int *)calloc(cube, sizeof(int));
    // assert(weight != NULL);
    weight[0] = 1;
    for (int j = 1; j < cube; ++j)
        weight[j] = ary * weight[j - 1];
    // return weight;
}

int cord_flatten(const int *cord, const int *weight, int cube)
{
    int idx = 0;
    for (int j = cube - 1; j >= 0; --j)
    {
        idx += cord[j] * weight[j];
    }
    return idx;
}

void cord_fold(int idx, const int *weight, int cube, int *cord)
{
    assert(cube > 0 && idx >= 0);
    // int *cord = (int *)calloc(cube, sizeof(int));
    // assert(cord != NULL);
    for (int j = cube - 1; j >= 0; --j)
    {
        cord[j] = idx / weight[j];
        idx = idx % weight[j];
    }
    // return cord;
}
