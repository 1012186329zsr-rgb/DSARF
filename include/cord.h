// tools for cord calculate
#ifndef __CORD_H__
#define __CORD_H__ 1

void calculate_weight(int ary, int cube, int *weight);

int cord_flatten(const int *cord, const int *weight, int cube);

void cord_fold(int idx, const int *weight, int cube, int *cord);

#endif