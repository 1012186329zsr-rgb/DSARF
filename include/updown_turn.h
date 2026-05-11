#ifndef __UPDOWN_TURN_H__
#define __UPDOWN_TURN_H__ 1

/*
 * Up/Down routing turn constraint.
 *
 * We implement Up/Down as a local turn restriction based on a total order key
 * derived from a spanning tree rooted at "root":
 *   key(node) = (-depth_from_root, tie)
 * where depth_from_root is from a BFS tree and tie is an arbitrary unique order
 * (we reuse TreeCoordinate.x from gen_tree_coordinate()).
 *
 * A hop is classified as:
 *   UP   if key(next) > key(curr)
 *   DOWN if key(next) < key(curr)
 *
 * The Up/Down rule forbids a DOWN->UP transition.
 */
int check_updown_turn(int x1, int y1, int x2, int y2, int x3, int y3);

#endif
