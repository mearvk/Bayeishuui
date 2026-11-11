#ifndef BAYEISHUUI_BARBER_H
#define BAYEISHUUI_BARBER_H

/*
 * BAYEISHUUI — the Barber Shop screen.
 *
 * A blocking, full-screen menu used by both native games (press H at the
 * Player 1 select screen, or any time during a bout). It runs inside the
 * games' existing raw, non-blocking terminal mode (VMIN=0, VTIME=0) and draws
 * a live, animated preview of your fighter wearing the highlighted style.
 *
 *   Up/Down, W/S, J/K   move          1..8     jump to a style
 *   Enter / Space       buy (if new) or equip
 *   U                   upgrade the highlighted owned style: Natural -> Dyed -> Glow
 *   T                   HAGGLE for the highlighted style (or its next upgrade)
 *   X                   TRADE IN the highlighted owned style, then pick the style you want
 *   Esc / Q / H / B     leave the shop
 *
 * Haggle mode:  Left/Right or -/+ change your offer by $50, Up/Down by $250,
 *               Enter submits it, A accepts the barber's counter, Esc/T walks away.
 *
 * Barter (T / X) is for MANUAL players: pass barter_ok = 0 for a Perfect Fighter
 * session, which pays sticker price. Money is in-game currency earned by
 * winning bouts. The profile is saved on every purchase, equip, upgrade and trade.
 *
 * Include order: bayeishuui_sprites.h, bayeishuui_pixels.h, then this header.
 */
#include <time.h>
#include <unistd.h>
#include "bayeishuui_pixels.h"
#include "bayeishuui_hair.h"
#include "bayeishuui_barter.h"

/* Non-blocking key read. Returns 0 (none), a character, 27 (lone Esc), or
 * 1001/1002/1003/1004 for the Up/Down/Right/Left arrow keys. */
static inline int bay_barber_read_key(void) {
    unsigned char b[8];
    ssize_t n = read(STDIN_FILENO, b, sizeof b);
    if (n <= 0) return 0;
    if (b[0] == 27) {
        if (n >= 3 && b[1] == '[') {
            if (b[2] == 'A') return 1001;
            if (b[2] == 'B') return 1002;
            if (b[2] == 'C') return 1003;
            if (b[2] == 'D') return 1004;
        }
        return 27;
    }
    return b[0];
}

static inline const char *bay_barber_status_text(int code) {
    switch (code) {
    case BAY_SHOP_OK:        return "Done.";
    case BAY_SHOP_BROKE:     return "Not enough money - win more bouts!";
    case BAY_SHOP_OWNED:     return "You already own that style.";
    case BAY_SHOP_MAXED:     return "Already fully upgraded (Glow).";
    case BAY_SHOP_NOT_OWNED: return "You do not own that style (Buzz Cut cannot be traded).";
    case BAY_BARTER_EQUIPPED:return "Equip another style first - you cannot trade the one you wear.";
    default:                 return "That is not available.";
    }
}

static inline void bay_barber_patience_dots(const BayBarter *bt, int item, char *out, size_t n) {
    char tmp[BAY_BARTER_PATIENCE + 1]; int pat = bt->patience[item];
    for (int i = 0; i < BAY_BARTER_PATIENCE; ++i) tmp[i] = i < pat ? '*' : '.';
    tmp[BAY_BARTER_PATIENCE] = '\0';
    snprintf(out, n, "%s", tmp);
}

static inline void bay_barber_shop(BayProfile *p, BayBackend backend, int width, int height, int barter_ok) {
    int cur = p->equipped, frame = 0;
    char msg[200];
    snprintf(msg, sizeof msg, "%s", barter_ok ? "Welcome. Pay sticker price - or haggle (T), or trade in (X)."
                                              : "Welcome. Perfect Fighter pays sticker price - play manually to barter.");
    const int sel0 = bay_hair_sel[1], lvl0 = bay_hair_lvl[1];
    int px = width - 14; if (px < 47) px = 47;

    BayBarter bt; bay_barter_begin(&bt, (unsigned)time(NULL) ^ ((unsigned)rand() << 8));
    int haggling = 0, hitem = 0, hstyle = 0, hupgrade = 0, trade_give = -1;
    long hlist = 0, hoffer = 0;

    for (;;) {
        /* ---- draw ---- */
        printf("\033[2J\033[H\033[38;2;130;205;255m  BAYEISHUUI  /  BARBER SHOP\033[0m\n");
        printf("\033[38;2;255;215;120m  WALLET: $%ld\033[0m   \033[38;2;155;190;220m(in-game money, earned by winning bouts)\033[0m\n\n", p->money);
        for (int i = 0; i < BAY_HAIR_COUNT; ++i) {
            const BayHairStyle *s = &bay_hair_styles[i];
            int owned = bay_hair_owned(p, i), lvl = owned ? p->level[i] : 0, r, g, b;
            bay_hair_color(i, lvl, frame, &r, &g, &b);
            printf("\033[%d;1H%s %d \033[38;2;%d;%d;%dm%-12s\033[0m ", 4 + i, i == cur ? "\033[1m>" : " ", i + 1, r, g, b, s->name);
            if (i == p->equipped)  printf("\033[38;2;95;235;155m[EQUIPPED]\033[0m %-7s", bay_hair_tier_name(lvl));
            else if (owned)        printf("\033[38;2;190;200;215m[owned]   \033[0m %-7s", bay_hair_tier_name(lvl));
            else                   printf("\033[38;2;255;215;120m$%-9ld\033[0m %-7s", s->price, "");
            if (i == trade_give)   printf(" \033[38;2;255;150;110m<- trade-in\033[0m");
            printf("\033[0m");
        }
        {
            const BayHairStyle *s = &bay_hair_styles[cur];
            long up = bay_hair_upgrade_price(p, cur);
            printf("\033[13;1H\033[38;2;220;225;235m  %s\033[0m", s->tagline);
            if (haggling) {
                char dots[16]; bay_barber_patience_dots(&bt, hitem, dots, sizeof dots);
                printf("\033[14;1H\033[38;2;255;150;110m  HAGGLE: %s%s  list $%ld\033[0m", hupgrade ? "upgrade " : "", s->name, hlist);
                printf("\033[15;1H\033[38;2;255;215;120m  Your offer: $%ld\033[0m   \033[38;2;190;200;215mpatience [%s]\033[0m", hoffer, dots);
                if (bt.counter[hitem] > 0)
                    printf("\033[17;1H\033[38;2;95;235;155m  Barber counters: $%ld   [A] accept\033[0m", bt.counter[hitem]);
            } else if (trade_give >= 0) {
                printf("\033[14;1H\033[38;2;255;150;110m  TRADE-IN: %s is worth $%ld - highlight a style you want and press Enter (X cancels)\033[0m",
                       bay_hair_styles[trade_give].name, bay_barter_trade_in_value(p, trade_give));
            } else if (bay_hair_owned(p, cur)) {
                if (up >= 0) printf("\033[14;1H\033[38;2;155;190;220m  [U] upgrade to %s: $%ld   [T] haggle\033[0m", bay_hair_tier_name(p->level[cur] + 1), up);
                else         printf("\033[14;1H\033[38;2;155;190;220m  Fully upgraded.\033[0m");
                long tv = bay_barter_trade_in_value(p, cur);
                if (tv > 0) printf("\033[15;1H\033[38;2;155;190;220m  [X] trade in for $%ld credit\033[0m", tv);
            } else {
                printf("\033[14;1H\033[38;2;155;190;220m  [Enter] buy for $%ld   [T] haggle\033[0m", s->price);
            }
        }
        printf("\033[16;1H\033[38;2;255;210;130m  %s\033[0m", msg);
        int hl = height - 1 > 20 ? height - 1 : 20;
        if (haggling)
            printf("\033[%d;1H\033[38;2;175;190;210m  Left/Right -/+ $50 | Up/Down $250 | Enter offer | A accept counter | Esc walk away\033[0m", hl);
        else
            printf("\033[%d;1H\033[38;2;175;190;210m  Up/Down or 1-8 | Enter buy/equip | U upgrade | T haggle | X trade-in | Esc leave\033[0m", hl);

        /* live preview of the highlighted style at the highlighted owned tier */
        bay_hair_sel[1] = cur;
        bay_hair_lvl[1] = bay_hair_owned(p, cur) ? p->level[cur] : 0;
        bay_pixel_draw_backend(backend, px, 5, 1, 0, frame, 0, 0, 0);
        fflush(stdout);

        /* ---- input ---- */
        struct timespec nap = {0, 60000000};
        nanosleep(&nap, NULL);
        ++frame;
        int k;
        while ((k = bay_barber_read_key()) != 0) {
            if (haggling) {                                   /* ---- haggle keys ---- */
                if (k == 27 || k == 't' || k == 'T') {
                    haggling = 0;
                    snprintf(msg, sizeof msg, "You step back from the counter.");
                } else if (k == 1003 || k == '+' || k == '=') { hoffer += 50; if (hoffer > hlist) hoffer = hlist; }
                else if (k == 1004 || k == '-' || k == '_')   { hoffer -= 50; if (hoffer < 10) hoffer = 10; }
                else if (k == 1001) { hoffer += 250; if (hoffer > hlist) hoffer = hlist; }
                else if (k == 1002) { hoffer -= 250; if (hoffer < 10) hoffer = 10; }
                else if (k == 'a' || k == 'A' || k == '\r' || k == '\n' || k == ' ') {
                    long price = 0; int rc;
                    if (k == 'a' || k == 'A') {
                        if (bt.counter[hitem] <= 0) { snprintf(msg, sizeof msg, "There is no counter-offer to accept yet."); continue; }
                        rc = BAY_BARTER_DEAL; price = bt.counter[hitem];
                    } else rc = bay_barter_offer(&bt, hitem, hlist, hoffer, &price);
                    if (rc == BAY_BARTER_DEAL) {
                        if (p->money < price) { snprintf(msg, sizeof msg, "Deal - but you only have $%ld. Offer less or earn more.", p->money); continue; }
                        p->money -= price;
                        if (hupgrade) { p->level[hstyle]++;
                            snprintf(msg, sizeof msg, "DEAL! %s upgraded to %s for $%ld (saved $%ld).", bay_hair_styles[hstyle].name, bay_hair_tier_name(p->level[hstyle]), price, hlist - price);
                        } else { p->owned |= 1 << hstyle; p->equipped = hstyle;
                            snprintf(msg, sizeof msg, "DEAL! %s is yours for $%ld (saved $%ld).", bay_hair_styles[hstyle].name, price, hlist - price);
                        }
                        bay_profile_save(p); haggling = 0;
                    } else if (rc == BAY_BARTER_COUNTER) {
                        hoffer = price;            /* Enter again takes the counter */
                        snprintf(msg, sizeof msg, "Barber: \"Too low, friend. How about $%ld?\"", price);
                    } else if (rc == BAY_BARTER_WALKOUT) {
                        snprintf(msg, sizeof msg, "The barber crosses his arms and walks off. No more haggling on this one today.");
                        haggling = 0;
                    } else { snprintf(msg, sizeof msg, "The barber will not talk about that any more today."); haggling = 0; }
                }
                continue;
            }
            if (k == 27 || k == 'q' || k == 'Q' || k == 'h' || k == 'H' || k == 'b' || k == 'B') goto leave;
            if (k == 1001 || k == 'w' || k == 'W' || k == 'k') cur = (cur + BAY_HAIR_COUNT - 1) % BAY_HAIR_COUNT;
            else if (k == 1002 || k == 's' || k == 'S' || k == 'j') cur = (cur + 1) % BAY_HAIR_COUNT;
            else if (k >= '1' && k < '1' + BAY_HAIR_COUNT) cur = k - '1';
            else if (k == '\r' || k == '\n' || k == ' ') {
                if (bay_hair_owned(p, cur)) {
                    bay_hair_equip(p, cur);
                    snprintf(msg, sizeof msg, "Equipped %s. Looking sharp!", bay_hair_styles[cur].name);
                } else if (trade_give >= 0) {
                    long due = 0; int give = trade_give;
                    int rc = bay_barter_trade_in(p, give, cur, &due);
                    if (rc == BAY_SHOP_OK) {
                        if (due >= 0) snprintf(msg, sizeof msg, "Traded in %s for %s - you paid $%ld.", bay_hair_styles[give].name, bay_hair_styles[cur].name, due);
                        else          snprintf(msg, sizeof msg, "Traded in %s for %s - the barber paid YOU $%ld!", bay_hair_styles[give].name, bay_hair_styles[cur].name, -due);
                        trade_give = -1;
                    } else snprintf(msg, sizeof msg, "%s", bay_barber_status_text(rc));
                } else {
                    int rc = bay_hair_buy(p, cur);
                    if (rc == BAY_SHOP_OK) {
                        bay_hair_equip(p, cur);
                        snprintf(msg, sizeof msg, "Bought %s for $%ld and equipped it!", bay_hair_styles[cur].name, bay_hair_styles[cur].price);
                    } else snprintf(msg, sizeof msg, "%s", bay_barber_status_text(rc));
                }
                bay_profile_save(p);
            } else if (k == 'u' || k == 'U') {
                long price = bay_hair_upgrade_price(p, cur);
                int rc = bay_hair_upgrade(p, cur);
                if (rc == BAY_SHOP_OK) snprintf(msg, sizeof msg, "Upgraded %s to %s for $%ld!", bay_hair_styles[cur].name, bay_hair_tier_name(p->level[cur]), price);
                else snprintf(msg, sizeof msg, "%s", bay_barber_status_text(rc));
                bay_profile_save(p);
            } else if (k == 't' || k == 'T') {
                if (!barter_ok) { snprintf(msg, sizeof msg, "Perfect Fighter pays sticker price. Play manually (double-tap SPACE at start) to haggle."); continue; }
                int owned = bay_hair_owned(p, cur);
                long list = owned ? bay_hair_upgrade_price(p, cur) : bay_hair_styles[cur].price;
                if (list < 0) { snprintf(msg, sizeof msg, "%s", bay_barber_status_text(BAY_SHOP_MAXED)); continue; }
                if (list == 0) { snprintf(msg, sizeof msg, "That one is free - nothing to haggle over."); continue; }
                hupgrade = owned; hstyle = cur; hlist = list;
                hitem = owned ? bay_barter_item_upgrade(cur) : bay_barter_item_buy(cur);
                if (bt.patience[hitem] <= 0) { snprintf(msg, sizeof msg, "The barber is done talking about that today."); continue; }
                hoffer = bay_barter_round(list * 60 / 100);
                if (hoffer < 10) hoffer = 10;
                haggling = 1; trade_give = -1;
                snprintf(msg, sizeof msg, "Barber: \"List is $%ld. Make me an offer.\"", list);
            } else if (k == 'x' || k == 'X') {
                if (!barter_ok) { snprintf(msg, sizeof msg, "Perfect Fighter pays sticker price. Play manually to trade in styles."); continue; }
                if (trade_give >= 0) { trade_give = -1; snprintf(msg, sizeof msg, "Trade-in cancelled."); }
                else if (!bay_hair_owned(p, cur) || cur == 0) snprintf(msg, sizeof msg, "%s", bay_barber_status_text(BAY_SHOP_NOT_OWNED));
                else if (cur == p->equipped) snprintf(msg, sizeof msg, "%s", bay_barber_status_text(BAY_BARTER_EQUIPPED));
                else { trade_give = cur; snprintf(msg, sizeof msg, "Offering %s ($%ld credit). Pick the style you want.", bay_hair_styles[cur].name, bay_barter_trade_in_value(p, cur)); }
            }
        }
    }
leave:
    bay_hair_sel[1] = sel0;
    bay_hair_lvl[1] = lvl0;
    bay_profile_save(p);
    printf("\033[2J\033[H");
    fflush(stdout);
}

#endif /* BAYEISHUUI_BARBER_H */
