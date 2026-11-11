#ifndef BAYEISHUUI_HAIR_H
#define BAYEISHUUI_HAIR_H

/*
 * BAYEISHUUI — hair styles, wallet and saved profile.
 *
 * Header-only (C11 + C++17). Cosmetic only: hair never changes combat odds.
 * The "$" is IN-GAME currency earned by winning bouts — there is no real-money
 * payment anywhere in this program.
 *
 * Styles are bought once, then equipped for free. Every owned style can be
 * UPGRADED twice: tier 1 "Dyed" (vivid colour) and tier 2 "Glow" (shimmering
 * colour). The renderers read bay_hair_sel[] / bay_hair_lvl[]:
 *   index 0 = rival, index 1 = player.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BAY_HAIR_COUNT      8
#define BAY_HAIR_MAX_LEVEL  2
#define BAY_HAIR_DYE_PRICE  500L
#define BAY_HAIR_GLOW_PRICE 1500L
#define BAY_START_MONEY     600L
#define BAY_MONEY_CAP       99999999L

typedef struct BayHairStyle {
    const char   *name;
    const char   *tagline;
    long          price;
    unsigned char r, g, b;        /* natural colour           */
    unsigned char vr, vg, vb;     /* vivid (dyed / glow) colour */
    const char   *glyph;          /* UTF-8 terminal glyph above the head */
    int           glyph_w;        /* glyph width in terminal cells       */
} BayHairStyle;

static const BayHairStyle bay_hair_styles[BAY_HAIR_COUNT] = {
    {"Buzz Cut",     "Regulation. Zero fuss.",                      0, 105, 80, 60, 200,200,210, "\xE2\x96\x92\xE2\x96\x92\xE2\x96\x92", 3},
    {"The Lover",    "Windswept and irresistible. Cheap, like love.", 800, 110, 60, 40, 255, 90,150, "\xE2\x89\x88\xE2\x99\xA5\xE2\x89\x88", 3},
    {"Topknot",      "Samurai discipline.",                       1500, 70, 66, 92,  60,200,255, "\xC2\xB7\xE2\x97\x8F\xC2\xB7", 3},
    {"The Man",      "Pay $2000 and be the man.",                 2000, 110, 70, 42, 230,200,120, "\xE2\x96\x9F\xE2\x96\x88\xE2\x96\x88\xE2\x96\x99", 4},
    {"Afro",         "Big. Bold. Unmissable.",                    2800, 85, 56, 40, 160, 90,255, "(\xE2\x96\x93\xE2\x96\x93\xE2\x96\x93)", 5},
    {"Punk",         "Pay $4500 and be a Punk!",                  4500, 255, 60,170,  60,255,120, "\xE2\x96\xB2\xE2\x96\xB2\xE2\x96\xB2", 3},
    {"Dragon Braid", "A braid with a legend behind it.",          6500, 80,105,150,  60,220,200, "\xE2\x89\x8B\xE2\x89\x8B\xE2\x89\x8B\xE2\x89\x8B", 4},
    {"Legend Mane",  "Worn by Guild Legends.",                    9000, 230,180, 50, 255,255,255, "\xE2\x9C\xA6\xE2\x96\x93\xE2\x9C\xA6", 3}
};

/* Live selection used by the renderers: [0] rival, [1] player. */
static int bay_hair_sel[2] = {0, 0};
static int bay_hair_lvl[2] = {0, 0};
/* Animation phase for the tier-2 shimmer; updated by the sprite skeleton. */
static int bay_hair_phase = 0;
/* Wallet mirrored for the HUD. */
static long bay_hud_money = 0;

static inline const char *bay_hair_tier_name(int lvl) { return lvl >= 2 ? "Glow" : (lvl == 1 ? "Dyed" : "Natural"); }

/* Resolve the colour of a style at an upgrade tier and animation phase. */
static inline void bay_hair_color(int sel, int lvl, int phase, int *r, int *g, int *b) {
    const BayHairStyle *s = &bay_hair_styles[(sel < 0 || sel >= BAY_HAIR_COUNT) ? 0 : sel];
    if (lvl <= 0) { *r = s->r; *g = s->g; *b = s->b; return; }
    if (lvl == 1) { *r = s->vr; *g = s->vg; *b = s->vb; return; }
    int ph = ((phase * 3) % 120 + 120) % 120;              /* triangle wave 0..60..0 */
    int t = ph < 60 ? ph : 120 - ph;                       /* 0..60 */
    int rr = s->vg, rg = s->vb, rb = s->vr;                /* channel-rotated twin  */
    *r = s->vr + (rr - s->vr) * t / 60 + (255 - s->vr) * t / 400;
    *g = s->vg + (rg - s->vg) * t / 60 + (255 - s->vg) * t / 400;
    *b = s->vb + (rb - s->vb) * t / 60 + (255 - s->vb) * t / 400;
    if (*r > 255) *r = 255;
    if (*g > 255) *g = 255;
    if (*b > 255) *b = 255;
}

/* Each Guild opponent wears a signature style (and fancier dye at high rank). */
static inline int bay_hair_rival_style(int rank) {
    static const int map[15] = {0, 3, 2, 5, 1, 4, 2, 5, 6, 3, 5, 1, 4, 6, 7};
    return map[(rank < 1 ? 1 : (rank > 15 ? 15 : rank)) - 1];
}
static inline int bay_hair_rival_level(int rank) { return rank >= 13 ? 2 : (rank >= 8 ? 1 : 0); }

/* ------------------------------------------------------------- profile */
typedef struct BayProfile {
    long money;
    int  owned;                       /* bitmask over BAY_HAIR_COUNT styles */
    int  equipped;
    int  level[BAY_HAIR_COUNT];       /* upgrade tier per style */
} BayProfile;

static inline void bay_profile_init(BayProfile *p) {
    memset(p, 0, sizeof *p);
    p->money = BAY_START_MONEY;
    p->owned = 1;                     /* Buzz Cut is free */
    p->equipped = 0;
}

static inline const char *bay_profile_path(char *buf, size_t n) {
    const char *e = getenv("BAYEISHUUI_PROFILE"), *home = getenv("HOME");
    if (e && *e) snprintf(buf, n, "%s", e);
    else if (home && *home) snprintf(buf, n, "%s/.bayeishuui-profile", home);
    else snprintf(buf, n, ".bayeishuui-profile");
    return buf;
}

/* Load the saved profile; on any problem the defaults are kept. Returns 1 if loaded. */
static inline int bay_profile_load(BayProfile *p) {
    bay_profile_init(p);
    char path[512]; bay_profile_path(path, sizeof path);
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    BayProfile q; bay_profile_init(&q);
    char line[128]; int ok = 0;
    while (fgets(line, sizeof line, f)) {
        long v; int i;
        if (!strncmp(line, "bayeishuui-profile-v1", 21)) ok = 1;
        else if (sscanf(line, "money=%ld", &v) == 1) q.money = v < 0 ? 0 : (v > BAY_MONEY_CAP ? BAY_MONEY_CAP : v);
        else if (sscanf(line, "owned=%ld", &v) == 1) q.owned = (int)(v & ((1 << BAY_HAIR_COUNT) - 1)) | 1;
        else if (sscanf(line, "equipped=%ld", &v) == 1) q.equipped = (int)v;
        else if (sscanf(line, "level%d=%ld", &i, &v) == 2 && i >= 0 && i < BAY_HAIR_COUNT)
            q.level[i] = v < 0 ? 0 : (v > BAY_HAIR_MAX_LEVEL ? BAY_HAIR_MAX_LEVEL : (int)v);
    }
    fclose(f);
    if (!ok) return 0;
    if (q.equipped < 0 || q.equipped >= BAY_HAIR_COUNT || !(q.owned & (1 << q.equipped))) q.equipped = 0;
    *p = q;
    return 1;
}

/* Save atomically (temp file + rename). Returns 1 on success. */
static inline int bay_profile_save(const BayProfile *p) {
    char path[512], tmp[560]; bay_profile_path(path, sizeof path);
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "w");
    if (!f) return 0;
    fprintf(f, "bayeishuui-profile-v1\nmoney=%ld\nowned=%d\nequipped=%d\n", p->money, p->owned, p->equipped);
    for (int i = 0; i < BAY_HAIR_COUNT; ++i) fprintf(f, "level%d=%d\n", i, p->level[i]);
    if (fclose(f) != 0) { remove(tmp); return 0; }
    if (rename(tmp, path) != 0) { remove(tmp); return 0; }
    return 1;
}

/* Prize money for a won bout. */
static inline long bay_profile_bout_reward(int rank, int arms_broken, int series_complete) {
    long r = 100 + 25L * rank + 150L * arms_broken;
    if (series_complete) r += 1000;
    return r;
}

/* ----------------------------------------------------------- shop rules */
enum { BAY_SHOP_OK = 0, BAY_SHOP_BROKE = 1, BAY_SHOP_OWNED = 2, BAY_SHOP_INVALID = 3, BAY_SHOP_MAXED = 4, BAY_SHOP_NOT_OWNED = 5 };

static inline int bay_hair_owned(const BayProfile *p, int id) { return id >= 0 && id < BAY_HAIR_COUNT && (p->owned & (1 << id)) != 0; }

/* Buy a style (does not equip). */
static inline int bay_hair_buy(BayProfile *p, int id) {
    if (id < 0 || id >= BAY_HAIR_COUNT) return BAY_SHOP_INVALID;
    if (bay_hair_owned(p, id)) return BAY_SHOP_OWNED;
    if (p->money < bay_hair_styles[id].price) return BAY_SHOP_BROKE;
    p->money -= bay_hair_styles[id].price;
    p->owned |= 1 << id;
    return BAY_SHOP_OK;
}

static inline int bay_hair_equip(BayProfile *p, int id) {
    if (id < 0 || id >= BAY_HAIR_COUNT) return BAY_SHOP_INVALID;
    if (!bay_hair_owned(p, id)) return BAY_SHOP_NOT_OWNED;
    p->equipped = id;
    return BAY_SHOP_OK;
}

static inline long bay_hair_upgrade_price(const BayProfile *p, int id) {
    if (id < 0 || id >= BAY_HAIR_COUNT || p->level[id] >= BAY_HAIR_MAX_LEVEL) return -1;
    return p->level[id] == 0 ? BAY_HAIR_DYE_PRICE : BAY_HAIR_GLOW_PRICE;
}

/* Upgrade an owned style one tier (Natural -> Dyed -> Glow). */
static inline int bay_hair_upgrade(BayProfile *p, int id) {
    if (id < 0 || id >= BAY_HAIR_COUNT) return BAY_SHOP_INVALID;
    if (!bay_hair_owned(p, id)) return BAY_SHOP_NOT_OWNED;
    long price = bay_hair_upgrade_price(p, id);
    if (price < 0) return BAY_SHOP_MAXED;
    if (p->money < price) return BAY_SHOP_BROKE;
    p->money -= price;
    p->level[id]++;
    return BAY_SHOP_OK;
}

/* HUD: wallet and current style, right of the STYLE row (row 3). Colour shows the tier. */
static inline void bay_hair_draw_hud(int width) {
    int sel = bay_hair_sel[1], lvl = bay_hair_lvl[1], r, g, b;
    bay_hair_color(sel, lvl, bay_hair_phase, &r, &g, &b);
    int col = width - 24; if (col < 37) col = 37;
    printf("\033[3;%dH\033[38;2;255;215;120m$%ld \033[38;2;%d;%d;%dm%s\033[0m", col, bay_hud_money, r, g, b,
           bay_hair_styles[sel].name);
}

#endif /* BAYEISHUUI_HAIR_H */
