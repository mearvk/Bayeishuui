#ifndef BAYEISHUUI_BARTER_H
#define BAYEISHUUI_BARTER_H

/*
 * BAYEISHUUI — barter: haggling and trade-ins at the Barber Shop.
 *
 * Header-only (C11 + C++17), pure and deterministic given a seed, so the rules
 * can be tested apart from the UI. Barter is open to MANUAL players; a Perfect
 * Fighter session pays sticker price (the UI enforces that).
 *
 * HAGGLING
 *   Every item (a style to buy, or an upgrade) has a hidden reserve price of
 *   70..84% of list, fixed for the shop visit by the visit seed so it cannot be
 *   re-rolled by reopening the haggle. The barber starts with 3 patience.
 *     offer >= reserve        -> DEAL at your offer (never above list)
 *     offer <  reserve        -> COUNTER, patience -1 (-2 if the offer is an
 *                                insult: under half the reserve); the counters
 *                                fall toward the reserve as patience drops
 *     patience reaches 0      -> WALKOUT: no more haggling on that item this visit
 *
 * TRADE-IN
 *   Swap an owned style (not Buzz Cut, not the equipped one) toward a new one.
 *   Credit = half the style's price + half of the upgrade money spent on it.
 *   Pay the difference (or pocket it if the credit is larger).
 */
#include "bayeishuui_hair.h"

#define BAY_BARTER_ITEMS     (2 * BAY_HAIR_COUNT)   /* buy-slots then upgrade-slots */
#define BAY_BARTER_PATIENCE  3
#define BAY_BARTER_STEP      10

enum { BAY_BARTER_DEAL = 0, BAY_BARTER_COUNTER = 1, BAY_BARTER_WALKOUT = 2, BAY_BARTER_LOCKED = 3, BAY_BARTER_EQUIPPED = 6 };

typedef struct BayBarter {
    unsigned seed;
    int      patience[BAY_BARTER_ITEMS];
    long     counter[BAY_BARTER_ITEMS];   /* barber's latest counter, 0 = none yet */
} BayBarter;

static inline long bay_barter_round(long x) { return ((x + BAY_BARTER_STEP / 2) / BAY_BARTER_STEP) * BAY_BARTER_STEP; }

/* Start a shop visit. */
static inline void bay_barter_begin(BayBarter *b, unsigned seed) {
    b->seed = seed;
    for (int i = 0; i < BAY_BARTER_ITEMS; ++i) { b->patience[i] = BAY_BARTER_PATIENCE; b->counter[i] = 0; }
}

/* Item slot: style purchase = style id; upgrade = BAY_HAIR_COUNT + style id. */
static inline int bay_barter_item_buy(int style) { return style; }
static inline int bay_barter_item_upgrade(int style) { return BAY_HAIR_COUNT + style; }

/* Hidden reserve price for an item, 70..84% of list (rounded to $10). */
static inline long bay_barter_reserve(const BayBarter *b, int item, long list) {
    unsigned h = b->seed ^ ((unsigned)(item + 1) * 2654435761u);
    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
    long pct = 70 + (long)(h % 15u);
    long r = bay_barter_round(list * pct / 100);
    if (r < BAY_BARTER_STEP) r = BAY_BARTER_STEP;
    if (r > list) r = list;
    return r;
}

/* Present an offer. On DEAL, *price is what to pay; on COUNTER it is the barber's counter. */
static inline int bay_barter_offer(BayBarter *b, int item, long list, long offer, long *price) {
    if (item < 0 || item >= BAY_BARTER_ITEMS || list <= 0) return BAY_BARTER_LOCKED;
    if (b->patience[item] <= 0) return BAY_BARTER_LOCKED;
    long reserve = bay_barter_reserve(b, item, list);
    if (offer >= reserve) { *price = offer > list ? list : offer; return BAY_BARTER_DEAL; }
    b->patience[item] -= (offer * 2 < reserve) ? 2 : 1;
    if (b->patience[item] <= 0) { b->patience[item] = 0; b->counter[item] = 0; return BAY_BARTER_WALKOUT; }
    long gap = list - reserve;
    long c = bay_barter_round(reserve + gap * (b->patience[item] - 1) / (BAY_BARTER_PATIENCE - 1));
    if (c < reserve) c = reserve;
    if (c > list) c = list;
    b->counter[item] = c;
    *price = c;
    return BAY_BARTER_COUNTER;
}

/* ------------------------------------------------------------- trade-in */
static inline long bay_barter_trade_in_value(const BayProfile *p, int id) {
    if (id <= 0 || id >= BAY_HAIR_COUNT || !bay_hair_owned(p, id)) return 0;
    long v = bay_hair_styles[id].price / 2;
    if (p->level[id] >= 1) v += BAY_HAIR_DYE_PRICE / 2;
    if (p->level[id] >= 2) v += BAY_HAIR_GLOW_PRICE / 2;
    return v;
}

/*
 * Give up `give` toward `want`. On success the new style is equipped (like a
 * normal purchase) and *due is what you paid (negative = you were paid).
 */
static inline int bay_barter_trade_in(BayProfile *p, int give, int want, long *due) {
    if (give < 0 || give >= BAY_HAIR_COUNT || want < 0 || want >= BAY_HAIR_COUNT) return BAY_SHOP_INVALID;
    if (!bay_hair_owned(p, give) || give == 0) return BAY_SHOP_NOT_OWNED;
    if (give == p->equipped) return BAY_BARTER_EQUIPPED;
    if (bay_hair_owned(p, want)) return BAY_SHOP_OWNED;
    long credit = bay_barter_trade_in_value(p, give);
    long d = bay_hair_styles[want].price - credit;
    if (d > 0 && p->money < d) return BAY_SHOP_BROKE;
    p->money -= d;                         /* d < 0 refunds the surplus */
    if (p->money > BAY_MONEY_CAP) p->money = BAY_MONEY_CAP;
    p->owned &= ~(1 << give);
    p->level[give] = 0;
    p->owned |= 1 << want;
    p->equipped = want;
    *due = d;
    return BAY_SHOP_OK;
}

#endif /* BAYEISHUUI_BARTER_H */
