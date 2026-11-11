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
 *   Esc / Q / H / B     leave the shop
 *
 * Money is in-game currency earned by winning bouts. The profile is saved on
 * every purchase, equip and upgrade.
 *
 * Include order: bayeishuui_sprites.h, bayeishuui_pixels.h, then this header.
 */
#include <time.h>
#include <unistd.h>
#include "bayeishuui_pixels.h"
#include "bayeishuui_hair.h"

/* Non-blocking key read. Returns 0 (none), a character, 27 (lone Esc), or
 * 1001/1002 for the Up/Down arrow keys. */
static inline int bay_barber_read_key(void) {
    unsigned char b[8];
    ssize_t n = read(STDIN_FILENO, b, sizeof b);
    if (n <= 0) return 0;
    if (b[0] == 27) {
        if (n >= 3 && b[1] == '[') {
            if (b[2] == 'A') return 1001;
            if (b[2] == 'B') return 1002;
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
    case BAY_SHOP_NOT_OWNED: return "Buy this style first.";
    default:                 return "That is not available.";
    }
}

static inline void bay_barber_shop(BayProfile *p, BayBackend backend, int width, int height) {
    int cur = p->equipped, frame = 0;
    char msg[160];
    snprintf(msg, sizeof msg, "Welcome to the Barber. Pick your look, fighter.");
    const int sel0 = bay_hair_sel[1], lvl0 = bay_hair_lvl[1];
    int px = width - 14; if (px < 47) px = 47;

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
            printf("\033[0m");
        }
        {
            const BayHairStyle *s = &bay_hair_styles[cur];
            long up = bay_hair_upgrade_price(p, cur);
            printf("\033[13;1H\033[38;2;220;225;235m  %s\033[0m", s->tagline);
            if (bay_hair_owned(p, cur)) {
                if (up >= 0) printf("\033[14;1H\033[38;2;155;190;220m  [U] upgrade to %s: $%ld\033[0m", bay_hair_tier_name(p->level[cur] + 1), up);
                else         printf("\033[14;1H\033[38;2;155;190;220m  Fully upgraded.\033[0m");
            } else {
                printf("\033[14;1H\033[38;2;155;190;220m  [Enter] buy for $%ld\033[0m", s->price);
            }
        }
        printf("\033[16;1H\033[38;2;255;210;130m  %s\033[0m", msg);
        printf("\033[%d;1H\033[38;2;175;190;210m  Up/Down or 1-8 choose | Enter buy/equip | U upgrade | Esc leave\033[0m", height - 1 > 18 ? height - 1 : 18);

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
            if (k == 27 || k == 'q' || k == 'Q' || k == 'h' || k == 'H' || k == 'b' || k == 'B') goto leave;
            if (k == 1001 || k == 'w' || k == 'W' || k == 'k') cur = (cur + BAY_HAIR_COUNT - 1) % BAY_HAIR_COUNT;
            else if (k == 1002 || k == 's' || k == 'S' || k == 'j') cur = (cur + 1) % BAY_HAIR_COUNT;
            else if (k >= '1' && k < '1' + BAY_HAIR_COUNT) cur = k - '1';
            else if (k == '\r' || k == '\n' || k == ' ') {
                if (bay_hair_owned(p, cur)) {
                    bay_hair_equip(p, cur);
                    snprintf(msg, sizeof msg, "Equipped %s. Looking sharp!", bay_hair_styles[cur].name);
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
