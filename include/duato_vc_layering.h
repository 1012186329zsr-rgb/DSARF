#ifndef __DUATO_VC_LAYERING_H__
#define __DUATO_VC_LAYERING_H__ 1

#include "route_sim_anytopo.h"

/* 控制是否启用 Duato hop-based VC 分层
 * 0: 关闭（沿用原有 VC 逻辑）
 * 1: 打开（hop1->VC0, hop2->VC1, hop3->VC2）
 */
#ifndef USE_DUATO_HOP_VC
#define USE_DUATO_HOP_VC 1
#endif

/* 注入阶段：给新包设定初始 hop 和 VC
 *  - R: 当前 RouterList
 *  - vc_random: 原来随机选的 VC
 *  返回：实际应该使用的 VC（Duato 模式下优先 0，否则用随机）
 */
static inline int duato_select_vc_on_inject(const RouterList *R, int vc_random)
{
#if USE_DUATO_HOP_VC
    if (R->vc_num >= 3) {
        return 0;  /* 第 1 跳 -> VC0 */
    } else {
        return vc_random;
    }
#else
    (void)R;
    return vc_random;
#endif
}

/* 转发阶段：根据 hop 决定下一跳 VC
 *  - R: RouterList
 *  - p_hop: 当前包已经走过的跳数
 *  - cur_vc: 当前所在 VC（用作默认值）
 *  返回：下一跳应使用的 VC
 */
static inline int duato_select_vc_on_forward(const RouterList *R, int p_hop, int cur_vc)
{
#if USE_DUATO_HOP_VC
    if (R->vc_num >= 3) {
        if (p_hop < R->vc_num)
            return p_hop;      /* hop=1->VC1, hop=2->VC2 */
        else
            return R->vc_num - 1;  /* 超出 3 跳：钳在最高 VC 上 */
    } else {
        (void)p_hop;
        return cur_vc;
    }
#else
    (void)R;
    (void)p_hop;
    return cur_vc;
#endif
}

/* Duato 模式下，在写 buffer_o 时更新 hop：
 *  - p_hop: 当前 hop
 *  返回：下一跳的 hop 值
 */
static inline int duato_next_hop(int p_hop)
{
#if USE_DUATO_HOP_VC
    return p_hop + 1;
#else
    (void)p_hop;
    return 0;
#endif
}

#endif /* __DUATO_VC_LAYERING_H__ */
