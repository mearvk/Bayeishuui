#ifndef BAYEISHUUI_PIXELS_H
#define BAYEISHUUI_PIXELS_H

/*
 * BAYEISHUUI — pixel-accurate fighter renderer for next-generation terminals.
 *
 * This is the high-fidelity path. Instead of drawing the interpolated skeleton
 * as fixed-width glyphs (see bayeishuui_sprites.h), it rasterizes the SAME
 * 12-joint skeleton into a true-colour RGBA bitmap — anti-aliased limbs with
 * real thickness, a shaded head, gloves, a drop shadow, and the per-style
 * colour identity — then ships that bitmap to the terminal as actual pixels.
 *
 * Transport, in order of fidelity:
 *   1. Kitty graphics protocol  (RGBA framebuffer, placed by cell, animated by
 *      image id)      — most accurate, preferred.
 *   2. Sixel                    (bitmap escape sequence, widely revived)
 *                      — accurate fallback.
 *   3. Glyph sprite             (bayeishuui_sprites.h) — universal fallback for
 *      plain TTYs / SSH to terminals without pixel support.
 *
 * No third-party libraries: the skeleton math is reused from the sprite header,
 * and the base64 / Sixel encoders are implemented here directly.
 *
 * The renderer keeps the SAME call signature as bay_sprite_draw() so the game
 * loop can pick a backend once and otherwise stay unchanged.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bayeishuui_sprites.h"

/* ------------------------------------------------------------------ config */
/* Each glyph cell becomes BAY_PX_CELL_W x BAY_PX_CELL_H device pixels. These
 * defaults assume a typical ~10x20 terminal cell, so a fighter fills roughly
 * the same area it does today but with smooth, sub-cell detail. */
#ifndef BAY_PX_CELL_W
#define BAY_PX_CELL_W 10
#endif
#ifndef BAY_PX_CELL_H
#define BAY_PX_CELL_H 20
#endif

#define BAY_PX_W (BAY_SPRITE_W * BAY_PX_CELL_W)   /* 110 px wide  by default */
#define BAY_PX_H (BAY_SPRITE_H * BAY_PX_CELL_H)   /* 140 px tall  by default */

typedef enum {
    BAY_BACKEND_UNKNOWN = 0,
    BAY_BACKEND_GLYPH,   /* bayeishuui_sprites.h   */
    BAY_BACKEND_SIXEL,   /* DEC sixel              */
    BAY_BACKEND_KITTY    /* kitty graphics protocol*/
} BayBackend;

/* -------------------------------------------------- capability detection */
/* Best-effort, non-blocking: honour explicit override, then well-known env
 * hints. We never probe stdin with a blocking query so the game start stays
 * snappy and SSH/pipe scenarios degrade cleanly to glyphs. */
static inline BayBackend bay_detect_backend(void) {
    const char *force = getenv("BAYEISHUUI_RENDERER");
    if (force && *force) {
        if (!strcmp(force, "kitty")) return BAY_BACKEND_KITTY;
        if (!strcmp(force, "sixel")) return BAY_BACKEND_SIXEL;
        if (!strcmp(force, "glyph")) return BAY_BACKEND_GLYPH;
    }
    if (!isatty(STDOUT_FILENO)) return BAY_BACKEND_GLYPH;

    const char *term    = getenv("TERM");
    const char *tprog   = getenv("TERM_PROGRAM");
    const char *kitty   = getenv("KITTY_WINDOW_ID");

    if (kitty) return BAY_BACKEND_KITTY;
    if (term  && strstr(term,  "kitty"))   return BAY_BACKEND_KITTY;
    if (tprog && (strstr(tprog, "WezTerm") || strstr(tprog, "ghostty")))
        return BAY_BACKEND_KITTY;
    if (tprog && strstr(tprog, "iTerm"))    return BAY_BACKEND_SIXEL;
    if (term  && (strstr(term, "foot") || strstr(term, "contour") ||
                  strstr(term, "mlterm") || strstr(term, "sixel") ||
                  strstr(term, "vt340")))   return BAY_BACKEND_SIXEL;
    return BAY_BACKEND_GLYPH;
}

/* ----------------------------------------------------------- RGBA raster */
typedef struct { unsigned char r, g, b, a; } BayRGBA;

static inline void bay_px_set(BayRGBA *fb, int x, int y,
                              unsigned char r, unsigned char g,
                              unsigned char b, unsigned char a) {
    if (x < 0 || x >= BAY_PX_W || y < 0 || y >= BAY_PX_H || a == 0) return;
    BayRGBA *d = &fb[y * BAY_PX_W + x];
    /* source-over alpha blend onto the existing pixel */
    unsigned ia = 255u - a;
    d->r = (unsigned char)((r * a + d->r * ia) / 255u);
    d->g = (unsigned char)((g * a + d->g * ia) / 255u);
    d->b = (unsigned char)((b * a + d->b * ia) / 255u);
    d->a = (unsigned char)(a + d->a * ia / 255u);
}

/* Filled, anti-aliased disc — the primitive for joints and limb caps. */
static inline void bay_px_disc(BayRGBA *fb, double cx, double cy, double rad,
                               unsigned char r, unsigned char g,
                               unsigned char b) {
    int x0 = (int)(cx - rad - 1), x1 = (int)(cx + rad + 1);
    int y0 = (int)(cy - rad - 1), y1 = (int)(cy + rad + 1);
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            double dx = x + 0.5 - cx, dy = y + 0.5 - cy;
            double d = rad - (dx*dx + dy*dy > 0 ? __builtin_sqrt(dx*dx + dy*dy) : 0);
            if (d <= -1.0) continue;
            double cov = d >= 0.0 ? 1.0 : d + 1.0;   /* 1px soft edge */
            bay_px_set(fb, x, y, r, g, b, (unsigned char)(cov * 255.0 + 0.5));
        }
}

/* Thick, round-capped, anti-aliased segment — the primitive for limbs. */
static inline void bay_px_capsule(BayRGBA *fb, double ax, double ay,
                                   double bx, double by, double half,
                                   unsigned char r, unsigned char g,
                                   unsigned char b) {
    double minx = (ax < bx ? ax : bx) - half - 1, maxx = (ax > bx ? ax : bx) + half + 1;
    double miny = (ay < by ? ay : by) - half - 1, maxy = (ay > by ? ay : by) + half + 1;
    double vx = bx - ax, vy = by - ay, len2 = vx*vx + vy*vy;
    for (int y = (int)miny; y <= (int)maxy; ++y)
        for (int x = (int)minx; x <= (int)maxx; ++x) {
            double px = x + 0.5 - ax, py = y + 0.5 - ay;
            double t = len2 > 0 ? (px*vx + py*vy) / len2 : 0.0;
            if (t < 0) t = 0; else if (t > 1) t = 1;
            double qx = px - t*vx, qy = py - t*vy;
            double dist = __builtin_sqrt(qx*qx + qy*qy);
            double d = half - dist;
            if (d <= -1.0) continue;
            double cov = d >= 0.0 ? 1.0 : d + 1.0;
            bay_px_set(fb, x, y, r, g, b, (unsigned char)(cov * 255.0 + 0.5));
        }
}

/* Rasterize the interpolated skeleton (joint coords in sprite-grid units)
 * into the RGBA framebuffer at BAY_PX_CELL scale. */
static inline void bay_px_rasterize(BayRGBA *fb, const int *v, int player,
                                    int style_id) {
    unsigned char br, bg, bb;              /* body colour */
    if (player) { br = 90; bg = 220; bb = 255; }
    else {
        static const unsigned char palette[15][3] = {
            {255,170,105},{245,130,95},{125,220,255},{255,190,100},{190,150,255},
            {255,115,135},{105,235,205},{200,220,255},{255,210,120},{130,190,255},
            {235,130,210},{255,155,115},{155,235,145},{235,220,155},{255,245,220}
        };
        int i = style_id < 0 ? 0 : style_id % 15;
        br = palette[i][0]; bg = palette[i][1]; bb = palette[i][2];
    }
    /* darker shade for shadow / outline, lighter for highlights */
    unsigned char dr = (unsigned char)(br*0.45), dg = (unsigned char)(bg*0.45), db = (unsigned char)(bb*0.45);

    /* map grid coords -> device pixels (centre of each cell) */
    #define PX(i) ((v[(i)] + 0.5) * BAY_PX_CELL_W)
    #define PY(i) ((v[(i)] + 0.5) * BAY_PX_CELL_H)
    double limb = BAY_PX_CELL_W * 0.42;    /* limb half-thickness */
    double torso = BAY_PX_CELL_W * 0.55;

    /* ground shadow ellipse (feet height) */
    double footY = (PY(19) > PY(23) ? PY(19) : PY(23)) + limb;
    for (int x = -(int)(BAY_PX_CELL_W*2); x <= (int)(BAY_PX_CELL_W*2); ++x) {
        double cov = 1.0 - (double)(x*x) / (double)((BAY_PX_CELL_W*2)*(BAY_PX_CELL_W*2));
        if (cov <= 0) continue;
        bay_px_set(fb, (int)PX(14)+x, (int)footY, 0,0,0, (unsigned char)(cov*70));
    }

    /* spine + torso */
    bay_px_capsule(fb, PX(2),PY(3), PX(14),PY(15), torso, br,bg,bb);
    /* shoulders */
    bay_px_capsule(fb, PX(2),PY(3), PX(4),PY(5), limb, br,bg,bb);
    /* arms: shoulder->elbow->wrist (left then right) */
    bay_px_capsule(fb, PX(4),PY(5), PX(6),PY(7),  limb, br,bg,bb);
    bay_px_capsule(fb, PX(6),PY(7), PX(8),PY(9),  limb, br,bg,bb);
    bay_px_capsule(fb, PX(4),PY(5), PX(10),PY(11),limb, br,bg,bb);
    bay_px_capsule(fb, PX(10),PY(11),PX(12),PY(13),limb, br,bg,bb);
    /* legs: hip->knee->foot (left then right) */
    bay_px_capsule(fb, PX(14),PY(15), PX(16),PY(17), limb, br,bg,bb);
    bay_px_capsule(fb, PX(16),PY(17), PX(18),PY(19), limb, br,bg,bb);
    bay_px_capsule(fb, PX(14),PY(15), PX(20),PY(21), limb, br,bg,bb);
    bay_px_capsule(fb, PX(20),PY(21), PX(22),PY(23), limb, br,bg,bb);

    /* gloves (wrists) — darker, slightly larger discs */
    bay_px_disc(fb, PX(8),PY(9),  limb*1.15, dr,dg,db);
    bay_px_disc(fb, PX(12),PY(13),limb*1.15, dr,dg,db);

    /* head: outline disc then lighter fill for a shaded look */
    double hx = PX(0), hy = PY(1), hr = BAY_PX_CELL_W*0.9;
    bay_px_disc(fb, hx, hy, hr, dr,dg,db);
    bay_px_disc(fb, hx, hy, hr*0.78, br,bg,bb);
    bay_px_disc(fb, hx - hr*0.3, hy - hr*0.3, hr*0.3,
                (unsigned char)(br+(255-br)*0.5),
                (unsigned char)(bg+(255-bg)*0.5),
                (unsigned char)(bb+(255-bb)*0.5)); /* highlight */
    #undef PX
    #undef PY
}

/* ------------------------------------------------------- base64 (kitty) */
static inline int bay_b64(const unsigned char *in, int n, char *out) {
    static const char t[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int o = 0;
    for (int i = 0; i < n; i += 3) {
        unsigned v = in[i] << 16;
        if (i+1 < n) v |= in[i+1] << 8;
        if (i+2 < n) v |= in[i+2];
        out[o++] = t[(v>>18)&63];
        out[o++] = t[(v>>12)&63];
        out[o++] = (i+1 < n) ? t[(v>>6)&63] : '=';
        out[o++] = (i+2 < n) ? t[v&63]      : '=';
    }
    out[o] = '\0';
    return o;
}

/* ---------------------------------------------------------- kitty output */
/* Place one fighter frame at terminal cell (col,row), 1-based like the ANSI
 * cursor the game already uses. The image is transmitted inline (RGBA, f=32)
 * in 4096-byte base64 chunks per the kitty protocol. */
static inline void bay_kitty_emit(const BayRGBA *fb, int col, int row) {
    int raw = BAY_PX_W * BAY_PX_H * 4;
    char *b64 = (char *)malloc((size_t)((raw + 2) / 3 * 4 + 1));
    if (!b64) return;
    bay_b64((const unsigned char *)fb, raw, b64);
    int total = (int)strlen(b64), pos = 0, first = 1;

    printf("\033[%d;%dH", row, col);           /* position the image */
    const int CH = 4096;
    while (pos < total) {
        int n = total - pos < CH ? total - pos : CH;
        int more = (pos + n) < total ? 1 : 0;
        if (first) {
            /* a=transmit+display, f=32 (RGBA), s/v = pixel dimensions */
            printf("\033_Ga=T,f=32,s=%d,v=%d,m=%d;", BAY_PX_W, BAY_PX_H, more);
            first = 0;
        } else {
            printf("\033_Gm=%d;", more);
        }
        fwrite(b64 + pos, 1, (size_t)n, stdout);
        printf("\033\\");
        pos += n;
    }
    free(b64);
}

/* ---------------------------------------------------------- sixel output */
/* Compact sixel emitter: quantises to a small on-the-fly palette and writes
 * band-by-band. Accurate enough for the fighter's flat-shaded colours. */
static inline void bay_sixel_emit(const BayRGBA *fb, int col, int row) {
    printf("\033[%d;%dH", row, col);
    printf("\033Pq");                                  /* start sixel */
    /* Register a tiny palette: transparent handled by skipping a=0 pixels. */
    /* We emit each distinct colour lazily via #<n>;2;r;g;b (percent scale). */
    /* For simplicity and determinism we down-sample colour to 6 bits/chan. */
    #define QUANT(c) ((int)((c) / 255.0 * 100.0 + 0.5))
    /* Build a small palette of up to 255 colours seen in the frame. */
    static int pr[256], pg[256], pb[256];
    int pal = 0;
    for (int i = 0; i < BAY_PX_W * BAY_PX_H; ++i) {
        if (fb[i].a < 128) continue;
        int r = QUANT(fb[i].r), g = QUANT(fb[i].g), b = QUANT(fb[i].b), found = 0;
        for (int k = 0; k < pal; ++k)
            if (pr[k]==r && pg[k]==g && pb[k]==b) { found = 1; break; }
        if (!found && pal < 256) { pr[pal]=r; pg[pal]=g; pb[pal]=b; ++pal; }
    }
    for (int k = 0; k < pal; ++k)
        printf("#%d;2;%d;%d;%d", k, pr[k], pg[k], pb[k]);

    for (int band = 0; band < BAY_PX_H; band += 6) {
        for (int k = 0; k < pal; ++k) {
            int wrote = 0;
            char *runline = (char *)malloc((size_t)BAY_PX_W + 1);
            if (!runline) break;
            for (int x = 0; x < BAY_PX_W; ++x) {
                int bits = 0;
                for (int dy = 0; dy < 6 && band+dy < BAY_PX_H; ++dy) {
                    BayRGBA p = fb[(band+dy)*BAY_PX_W + x];
                    if (p.a >= 128 && QUANT(p.r)==pr[k] &&
                        QUANT(p.g)==pg[k] && QUANT(p.b)==pb[k])
                        bits |= 1 << dy;
                }
                runline[x] = (char)(0x3f + bits);
                if (bits) wrote = 1;
            }
            runline[BAY_PX_W] = '\0';
            if (wrote) { printf("#%d", k); fwrite(runline, 1, (size_t)BAY_PX_W, stdout); printf("$"); }
            free(runline);
        }
        printf("-");                                   /* next sixel band */
    }
    printf("\033\\");                                  /* end sixel */
    #undef QUANT
}

/* ----------------------------------------------------- unified entry point */
/* Same signature as bay_sprite_draw(). (x,y) is the 1-based terminal cell
 * where the fighter's top-left begins, matching the glyph renderer. */
static inline void bay_pixel_draw_backend(BayBackend backend,
                                          int x, int y, int player, int style,
                                          int frame, int guarding, int striking,
                                          int ground_mode) {
    if (backend == BAY_BACKEND_GLYPH || backend == BAY_BACKEND_UNKNOWN) {
        bay_sprite_draw(x, y, player, style, frame, guarding, striking, ground_mode);
        return;
    }
    int v[24], style_id = style < 0 ? 0 : style % 15;
    bay_sprite_skeleton(v, player, style, frame, guarding, striking, ground_mode);

    BayRGBA *fb = (BayRGBA *)calloc((size_t)(BAY_PX_W * BAY_PX_H), sizeof(BayRGBA));
    if (!fb) { bay_sprite_draw(x, y, player, style, frame, guarding, striking, ground_mode); return; }
    bay_px_rasterize(fb, v, player, style_id);

    if (backend == BAY_BACKEND_KITTY) bay_kitty_emit(fb, x, y);
    else                              bay_sixel_emit(fb, x, y);
    free(fb);
}

#endif /* BAYEISHUUI_PIXELS_H */
