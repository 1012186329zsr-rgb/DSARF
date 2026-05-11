#include "updown_turn.h"

/*
 * Coordinates convention (from gen_tree_coordinate):
 *   x: preorder index (unique ordering)
 *   y: BFS depth from chosen root
 *
 * We define an ordering key where larger means "more up" (closer to root).
 */
static inline int key_y(int y) { return -y; }

static inline int is_up(int x_from, int y_from, int x_to, int y_to)
{
    if (key_y(y_to) > key_y(y_from)) return 1;
    if (key_y(y_to) < key_y(y_from)) return 0;
    return x_to > x_from;
}

static inline int is_down(int x_from, int y_from, int x_to, int y_to)
{
    if (key_y(y_to) < key_y(y_from)) return 1;
    if (key_y(y_to) > key_y(y_from)) return 0;
    return x_to < x_from;
}

int check_updown_turn(int x1, int y1, int x2, int y2, int x3, int y3)
{
    /* Forbid DOWN->UP turns */
    if (is_down(x1, y1, x2, y2) && is_up(x2, y2, x3, y3))
        return 0;
    return 1;
}
