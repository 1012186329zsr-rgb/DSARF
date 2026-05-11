#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "l_turn.h"
#include "floyd_warshall.h"
#include "omp.h"

int l_turn_is_lu(int x1, int y1, int x2, int y2)
{
    return (x2 < x1) && (y2 < y1);
}

int l_turn_is_ld(int x1, int y1, int x2, int y2)
{
    return (x2 < x1) && (y2 >= y1);
}

int l_turn_is_ru(int x1, int y1, int x2, int y2)
{
    return (x2 > x1) && (y2 <= y1);
}
int l_turn_is_rd(int x1, int y1, int x2, int y2)
{
    return (x2 > x1) && (y2 > y1);
}

int check_l_trun(int x1, int y1, int x2, int y2, int x3, int y3)
{
    if ((l_turn_is_ld(x1, y1, x2, y2) && l_turn_is_lu(x2, y2, x3, y3)) ||
        (l_turn_is_ru(x1, y1, x2, y2) && l_turn_is_lu(x2, y2, x3, y3)) ||
        (l_turn_is_rd(x1, y1, x2, y2) && l_turn_is_lu(x2, y2, x3, y3)) ||
        (l_turn_is_ld(x1, y1, x2, y2) && l_turn_is_ru(x2, y2, x3, y3)) ||
        (l_turn_is_ld(x1, y1, x2, y2) && l_turn_is_rd(x2, y2, x3, y3)))
        return 0;
    else
        return 1;
}