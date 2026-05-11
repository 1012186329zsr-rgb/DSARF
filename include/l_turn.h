#ifndef __L_TURN_H__
#define __L_TURN_H__ 1

#include "tree_turn.h"

int l_turn_is_lu(int x1, int y1, int x2, int y2);
int l_turn_is_ld(int x1, int y1, int x2, int y2);
int l_turn_is_ru(int x1, int y1, int x2, int y2);
int l_turn_is_rd(int x1, int y1, int x2, int y2);

int check_l_trun(int x1, int y1, int x2, int y2, int x3, int y3);

#endif