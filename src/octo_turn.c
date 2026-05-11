#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "octo_turn.h"
#include "floyd_warshall.h"
#include "omp.h"

int octo_turn_is_u(int x1, int y1, int x2, int y2)
{
    return (x2 + 1 == x1) && (y2 < y1);
}

int octo_turn_is_d(int x1, int y1, int x2, int y2)
{
    return (x2 == x1 + 1) && (y2 > y1);
}

int octo_turn_is_l(int x1, int y1, int x2, int y2)
{
    return (x2 < x1) && (y2 == y1);
}

int octo_turn_is_r(int x1, int y1, int x2, int y2)
{
    return (x2 > x1) && (y2 == y1);
}

int octo_turn_is_lu(int x1, int y1, int x2, int y2)
{
    return (x2 + 1 < x1) && (y2 < y1);
}

int octo_turn_is_ld(int x1, int y1, int x2, int y2)
{
    return (x2 < x1) && (y2 > y1);
}

int octo_turn_is_ru(int x1, int y1, int x2, int y2)
{
    return (x2 > x1) && (y2 < y1);
}
int octo_turn_is_rd(int x1, int y1, int x2, int y2)
{
    return (x2 > x1 + 1) && (y2 > y1);
}

// int check_octo_trun(int x1, int y1, int x2, int y2, int x3, int y3)
// {
//     if ((octo_turn_is_l(x1, y1, x2, y2) && octo_turn_is_lu(x2, y2, x3, y3)) ||
//         (octo_turn_is_ld(x1, y1, x2, y2) && octo_turn_is_lu(x2, y2, x3, y3)) ||
//         (octo_turn_is_ru(x1, y1, x2, y2) && octo_turn_is_lu(x2, y2, x3, y3)) ||
//         (octo_turn_is_r(x1, y1, x2, y2) && octo_turn_is_lu(x2, y2, x3, y3)) ||
//         (octo_turn_is_rd(x1, y1, x2, y2) && octo_turn_is_lu(x2, y2, x3, y3)) ||
//         (octo_turn_is_ru(x1, y1, x2, y2) && octo_turn_is_l(x2, y2, x3, y3)) ||
//         (octo_turn_is_r(x1, y1, x2, y2) && octo_turn_is_l(x2, y2, x3, y3)) ||
//         (octo_turn_is_ru(x1, y1, x2, y2) && octo_turn_is_ld(x2, y2, x3, y3)) ||
//         (octo_turn_is_ru(x1, y1, x2, y2) && octo_turn_is_r(x2, y2, x3, y3)) ||
//         (octo_turn_is_ru(x1, y1, x2, y2) && octo_turn_is_rd(x2, y2, x3, y3)) ||
//         (octo_turn_is_rd(x1, y1, x2, y2) && octo_turn_is_u(x2, y2, x3, y3)) || // addition
//         (octo_turn_is_l(x1, y1, x2, y2) && octo_turn_is_u(x2, y2, x3, y3)) ||
//         (octo_turn_is_ru(x1, y1, x2, y2) && octo_turn_is_u(x2, y2, x3, y3)) ||
//         (octo_turn_is_r(x1, y1, x2, y2) && octo_turn_is_u(x2, y2, x3, y3)) ||
//         (octo_turn_is_ld(x1, y1, x2, y2) && octo_turn_is_u(x2, y2, x3, y3)) ||
//         (octo_turn_is_d(x1, y1, x2, y2) && octo_turn_is_lu(x2, y2, x3, y3)) ||
//         (octo_turn_is_ru(x1, y1, x2, y2) && octo_turn_is_d(x2, y2, x3, y3)) ||
//         (octo_turn_is_d(x1, y1, x2, y2) && octo_turn_is_u(x2, y2, x3, y3)))
//         return 0;
//     else
//         return 1;
// }

int check_octo_trun(int x1, int y1, int x2, int y2, int x3, int y3)
{
    if ((octo_turn_is_l(x1, y1, x2, y2) && octo_turn_is_lu(x2, y2, x3, y3)) ||
        (octo_turn_is_ld(x1, y1, x2, y2) && octo_turn_is_lu(x2, y2, x3, y3)) ||
        (octo_turn_is_ru(x1, y1, x2, y2) && octo_turn_is_lu(x2, y2, x3, y3)) ||
        (octo_turn_is_r(x1, y1, x2, y2) && octo_turn_is_lu(x2, y2, x3, y3)) ||
        (octo_turn_is_rd(x1, y1, x2, y2) && octo_turn_is_lu(x2, y2, x3, y3)) ||
        (octo_turn_is_ru(x1, y1, x2, y2) && octo_turn_is_l(x2, y2, x3, y3)) ||
        (octo_turn_is_r(x1, y1, x2, y2) && octo_turn_is_l(x2, y2, x3, y3)) ||
        (octo_turn_is_ru(x1, y1, x2, y2) && octo_turn_is_ld(x2, y2, x3, y3)) ||
        (octo_turn_is_ru(x1, y1, x2, y2) && octo_turn_is_r(x2, y2, x3, y3)) ||
        (octo_turn_is_ru(x1, y1, x2, y2) && octo_turn_is_rd(x2, y2, x3, y3)) ||
        (octo_turn_is_rd(x1, y1, x2, y2) && octo_turn_is_u(x2, y2, x3, y3)) || // addition
        (octo_turn_is_u(x1, y1, x2, y2) && octo_turn_is_l(x2, y2, x3, y3)) ||
        (octo_turn_is_l(x1, y1, x2, y2) && octo_turn_is_u(x2, y2, x3, y3)) ||
        (octo_turn_is_u(x1, y1, x2, y2) && octo_turn_is_ld(x2, y2, x3, y3)) ||
        (octo_turn_is_ld(x1, y1, x2, y2) && octo_turn_is_u(x2, y2, x3, y3)) ||
        (octo_turn_is_d(x1, y1, x2, y2) && octo_turn_is_lu(x2, y2, x3, y3)) || // D
        (octo_turn_is_ru(x1, y1, x2, y2) && octo_turn_is_d(x2, y2, x3, y3)) ||
        (octo_turn_is_d(x1, y1, x2, y2) && octo_turn_is_u(x2, y2, x3, y3)))
        return 0;
    else
        return 1;
}