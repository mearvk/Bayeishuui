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
#define BAY_PX_CELL_W 20
#endif
#ifndef BAY_PX_CELL_H
#define BAY_PX_CELL_H 40
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

/* ------------------------------------------------- shaded primitives */
/* A clamped 0..255 helper for blended colour channels. */
static inline unsigned char bay_px_clampc(double x) {
    return (unsigned char)(x < 0 ? 0 : x > 255 ? 255 : x + 0.5);
}

/* Volumetric capsule: a round-capped segment shaded by a fixed top-left key
 * light, with a brighter rim along the lit edge and a soft dark underside, so
 * limbs read as rounded 3D tubes rather than flat bars. */
static inline void bay_px_capsule_shaded(BayRGBA *fb, double ax, double ay,
                                         double bx, double by, double half,
                                         unsigned char r, unsigned char g,
                                         unsigned char b) {
    /* light direction (normalised), pointing from upper-left */
    const double lx = -0.55, ly = -0.83;
    double minx = (ax < bx ? ax : bx) - half - 2, maxx = (ax > bx ? ax : bx) + half + 2;
    double miny = (ay < by ? ay : by) - half - 2, maxy = (ay > by ? ay : by) + half + 2;
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
            /* surface normal across the tube (perpendicular offset / radius) */
            double nx = half > 0 ? qx / half : 0, ny = half > 0 ? qy / half : 0;
            double lambert = nx*lx + ny*ly;           /* -1..1 */
            double shade = 0.62 + 0.46 * lambert;      /* ambient + diffuse */
            if (shade < 0.4) shade = 0.4;
            /* rim: bright thin band where the lit edge meets the silhouette */
            double edge = dist / half;                  /* 0 centre .. 1 rim */
            double rim = (lambert > 0.25 && edge > 0.72) ? (edge - 0.72) * 2.6 * lambert : 0.0;
            double cr = r*shade + (255 - r)*rim;
            double cg = g*shade + (255 - g)*rim;
            double cb = b*shade + (255 - b)*rim;
            bay_px_set(fb, x, y, bay_px_clampc(cr), bay_px_clampc(cg),
                       bay_px_clampc(cb), (unsigned char)(cov * 255.0 + 0.5));
        }
}

/* Volumetric sphere: shaded disc with key light + specular hotspot. */
static inline void bay_px_sphere(BayRGBA *fb, double cx, double cy, double rad,
                                 unsigned char r, unsigned char g,
                                 unsigned char b) {
    const double lx = -0.55, ly = -0.70, lz = 0.45;   /* light has +z so we get a hotspot */
    int x0 = (int)(cx - rad - 2), x1 = (int)(cx + rad + 2);
    int y0 = (int)(cy - rad - 2), y1 = (int)(cy + rad + 2);
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            double dx = (x + 0.5 - cx) / rad, dy = (y + 0.5 - cy) / rad;
            double rr = dx*dx + dy*dy;
            double d = rad - __builtin_sqrt((x+0.5-cx)*(x+0.5-cx) + (y+0.5-cy)*(y+0.5-cy));
            if (d <= -1.0) continue;
            double cov = d >= 0.0 ? 1.0 : d + 1.0;
            double nz = rr < 1.0 ? __builtin_sqrt(1.0 - rr) : 0.0;
            double lambert = dx*lx + dy*ly + nz*lz;
            double shade = 0.5 + 0.55 * (lambert > 0 ? lambert : 0);
            /* specular hotspot */
            double spec = lambert > 0.86 ? (lambert - 0.86) * 7.0 : 0.0;
            double cr = r*shade + (255 - r)*spec;
            double cg = g*shade + (255 - g)*spec;
            double cb = b*shade + (255 - b)*spec;
            bay_px_set(fb, x, y, bay_px_clampc(cr), bay_px_clampc(cg),
                       bay_px_clampc(cb), (unsigned char)(cov * 255.0 + 0.5));
        }
}

/* Soft radial ground shadow (wide, short ellipse). */
static inline void bay_px_ground_shadow(BayRGBA *fb, double cx, double cy,
                                         double rx, double ry) {
    for (int y = (int)(cy - ry - 1); y <= (int)(cy + ry + 1); ++y)
        for (int x = (int)(cx - rx - 1); x <= (int)(cx + rx + 1); ++x) {
            double dx = (x + 0.5 - cx) / rx, dy = (y + 0.5 - cy) / ry;
            double r = dx*dx + dy*dy;
            if (r >= 1.0) continue;
            bay_px_set(fb, x, y, 0, 0, 0, (unsigned char)((1.0 - r) * 90));
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
    int sid = style_id < 0 ? 0 : style_id % 15;
    /* Per-fighter GLOVE and SHORTS colours — bold, fixed, high-contrast
     * against the body so the Muay Thai kit actually reads. Not derived from
     * the body colour (that washed out on same-hue fighters). */
    static const unsigned char glove_pal[15][3] = {
        {220,40,45},{40,90,230},{230,60,60},{250,180,30},{150,40,230},
        {230,40,120},{30,180,160},{40,80,210},{235,120,20},{40,110,240},
        {210,40,180},{230,70,40},{60,190,70},{210,170,30},{235,235,235}
    };
    static const unsigned char short_pal[15][3] = {
        {30,40,110},{210,40,45},{30,70,160},{120,30,140},{235,200,40},
        {30,120,110},{150,40,50},{230,120,30},{40,60,150},{200,40,60},
        {40,120,60},{40,80,170},{200,50,120},{60,60,160},{40,50,120}
    };
    unsigned char gr, gg, gb, sr, sg, sb;
    if (player) { gr=235; gg=235; gb=235;  sr=40; sg=90; sb=235; }   /* white gloves, blue trunks */
    else { gr=glove_pal[sid][0]; gg=glove_pal[sid][1]; gb=glove_pal[sid][2];
           sr=short_pal[sid][0]; sg=short_pal[sid][1]; sb=short_pal[sid][2]; }
    /* (silhouette outline colour reserved for a future pass) */
    /* Skin tone for the head, tinted slightly toward the identity colour. */
    unsigned char kr = bay_px_clampc(224 + (br-180)*0.08);
    unsigned char kg = bay_px_clampc(176 + (bg-180)*0.08);
    unsigned char kb = bay_px_clampc(150 + (bb-180)*0.08);

    /* map grid coords -> device pixels (centre of each cell) */
    #define PX(i) ((v[(i)] + 0.5) * BAY_PX_CELL_W)
    #define PY(i) ((v[(i)] + 0.5) * BAY_PX_CELL_H)
    double limb  = BAY_PX_CELL_W * 0.40;   /* arm/leg half-thickness */
    double thigh = BAY_PX_CELL_W * 0.48;   /* legs are a touch thicker */
    double torso = BAY_PX_CELL_W * 0.62;   /* chest mass */

    /* soft radial ground shadow under the feet */
    double footY = (PY(19) > PY(23) ? PY(19) : PY(23)) + limb*0.6;
    bay_px_ground_shadow(fb, PX(14), footY, BAY_PX_CELL_W*2.2, BAY_PX_CELL_H*0.35);

    /* legs first (behind torso): hip->knee->foot, left then right */
    bay_px_capsule_shaded(fb, PX(14),PY(15), PX(16),PY(17), thigh, br,bg,bb);
    bay_px_capsule_shaded(fb, PX(16),PY(17), PX(18),PY(19), limb,  br,bg,bb);
    bay_px_capsule_shaded(fb, PX(14),PY(15), PX(20),PY(21), thigh, br,bg,bb);
    bay_px_capsule_shaded(fb, PX(20),PY(21), PX(22),PY(23), limb,  br,bg,bb);

    /* spine + chest mass */
    bay_px_capsule_shaded(fb, PX(2),PY(3), PX(14),PY(15), torso, br,bg,bb);
    /* shoulders / collar line */
    bay_px_capsule_shaded(fb, PX(2),PY(3), PX(4),PY(5), torso*0.8, br,bg,bb);

    /* Muay Thai trunks: a bold coloured block over the hips with short legs
     * down each thigh and a white waistband trim on top, so the kit reads
     * clearly even when the body shares the fighter's hue. */
    double hipw = BAY_PX_CELL_W * 0.85;
    bay_px_capsule_shaded(fb, PX(14)-hipw, PY(15)+BAY_PX_CELL_H*0.08,
                              PX(14)+hipw, PY(15)+BAY_PX_CELL_H*0.08,
                              torso*0.82, sr,sg,sb);
    bay_px_capsule_shaded(fb, PX(14),PY(15)+BAY_PX_CELL_H*0.05, PX(14)*0.35+PX(16)*0.65, PY(15)*0.35+PY(17)*0.65, thigh*1.18, sr,sg,sb);
    bay_px_capsule_shaded(fb, PX(14),PY(15)+BAY_PX_CELL_H*0.05, PX(14)*0.35+PX(20)*0.65, PY(15)*0.35+PY(21)*0.65, thigh*1.18, sr,sg,sb);
    /* white waistband trim */
    bay_px_capsule(fb, PX(14)-hipw, PY(15)-BAY_PX_CELL_H*0.02,
                       PX(14)+hipw, PY(15)-BAY_PX_CELL_H*0.02,
                       BAY_PX_CELL_H*0.035, 240,240,245);

    /* arms: shoulder->elbow->wrist (left then right) */
    bay_px_capsule_shaded(fb, PX(4),PY(5), PX(6),PY(7),   limb, br,bg,bb);
    bay_px_capsule_shaded(fb, PX(6),PY(7), PX(8),PY(9),   limb, br,bg,bb);
    bay_px_capsule_shaded(fb, PX(4),PY(5), PX(10),PY(11), limb, br,bg,bb);
    bay_px_capsule_shaded(fb, PX(10),PY(11),PX(12),PY(13),limb, br,bg,bb);

    /* boxing gloves at the wrists: big coloured sphere (per fighter) + a white
     * wrist cuff and a bright specular, so they look like real gloves. */
    for (int wi = 0; wi < 2; ++wi) {
        double wx = wi ? PX(12) : PX(8), wy = wi ? PY(13) : PY(9);
        double ex = wi ? PX(10) : PX(6), ey = wi ? PY(11) : PY(7);
        double ang = __builtin_atan2(wy - ey, wx - ex);
        /* white cuff just behind the glove, toward the forearm */
        bay_px_disc(fb, wx - __builtin_cos(ang)*limb*0.9,
                        wy - __builtin_sin(ang)*limb*0.9, limb*0.8, 235,235,240);
        bay_px_sphere(fb, wx, wy, limb*1.55, gr,gg,gb);
    }

    /* neck */
    bay_px_capsule_shaded(fb, PX(2),PY(3), PX(0),PY(1)+BAY_PX_CELL_H*0.3,
                              BAY_PX_CELL_W*0.32, kr,kg,kb);

    /* head: shaded skin sphere */
    double hx = PX(0), hy = PY(1), hr = BAY_PX_CELL_W*0.92;
    bay_px_sphere(fb, hx, hy, hr, kr,kg,kb);
    /* mongkhon / headband: a bold ring across the brow in the glove colour,
     * wrapped onto the head sphere, with two knot tails behind. */
    for (int yy = (int)(hy - hr*0.62); yy <= (int)(hy - hr*0.08); ++yy)
        for (int xx = (int)(hx - hr); xx <= (int)(hx + hr); ++xx) {
            double dx = (xx + 0.5 - hx) / hr, dy = (yy + 0.5 - hy) / hr;
            double rr = dx*dx + dy*dy;
            if (rr >= 0.95) continue;
            double nz = __builtin_sqrt(1.0 - rr);
            double shade = 0.62 + 0.5 * (dx*-0.55 + dy*-0.70 + nz*0.45);
            if (shade < 0.45) shade = 0.45;
            bay_px_set(fb, xx, yy, bay_px_clampc(gr*shade),
                       bay_px_clampc(gg*shade), bay_px_clampc(gb*shade), 255);
        }
    /* knot tails trailing off the side of the head */
    bay_px_capsule(fb, hx - hr*0.85, hy - hr*0.3, hx - hr*1.4, hy + hr*0.25,
                   hr*0.14, gr,gg,gb);
    bay_px_capsule(fb, hx - hr*0.85, hy - hr*0.1, hx - hr*1.5, hy + hr*0.5,
                   hr*0.12, gr,gg,gb);
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

/* Gradient dojo backdrop: a soft vertical wash (deep floor -> lighter air)
 * with a faint radial glow centred on the fighter, so pixels have a stage. */
static inline void bay_px_backdrop(BayRGBA *fb) {
    for (int y = 0; y < BAY_PX_H; ++y) {
        double t = (double)y / (BAY_PX_H - 1);         /* 0 top .. 1 bottom */
        /* top air is cool indigo, floor is a warmer dark */
        double r = 16 + (1.0 - t) * 10 + t * 14;
        double g = 20 + (1.0 - t) * 14 + t * 10;
        double b = 34 + (1.0 - t) * 26 - t * 6;
        for (int x = 0; x < BAY_PX_W; ++x) {
            double dx = (x - BAY_PX_W * 0.5) / (BAY_PX_W * 0.5);
            double dy = (y - BAY_PX_H * 0.42) / (BAY_PX_H * 0.5);
            double glow = 1.0 - (dx*dx + dy*dy);        /* centre spotlight */
            if (glow < 0) glow = 0;
            BayRGBA *d = &fb[y * BAY_PX_W + x];
            d->r = bay_px_clampc(r + glow * 26);
            d->g = bay_px_clampc(g + glow * 24);
            d->b = bay_px_clampc(b + glow * 30);
            d->a = 255;
        }
    }
}

/* Motion-blur trail: re-rasterize the fighter at a couple of slightly earlier
 * frames into the same framebuffer at low alpha, so a strike leaves a sweep.
 * Reuses the shared skeleton, so the trail tracks real motion. */
static inline void bay_px_motion_trail(BayRGBA *fb, int player, int style,
                                       int frame, int guarding, int striking,
                                       int ground_mode, int style_id) {
    if (!striking) return;
    const int steps[2] = { 6, 12 };                     /* frames back */
    const unsigned char al[2] = { 110, 55 };            /* ghost opacity */
    for (int s = 0; s < 2; ++s) {
        int tv[24];
        bay_sprite_skeleton(tv, player, style, frame - steps[s],
                            guarding, striking, ground_mode);
        BayRGBA *ghost = (BayRGBA *)calloc((size_t)(BAY_PX_W * BAY_PX_H), sizeof(BayRGBA));
        if (!ghost) return;
        bay_px_rasterize(ghost, tv, player, style_id);
        for (int i = 0; i < BAY_PX_W * BAY_PX_H; ++i)
            if (ghost[i].a) {
                unsigned a = ghost[i].a * al[s] / 255u;
                bay_px_set(fb, i % BAY_PX_W, i / BAY_PX_W,
                           ghost[i].r, ghost[i].g, ghost[i].b, (unsigned char)a);
            }
        free(ghost);
    }
}

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
    bay_px_backdrop(fb);                                 /* dojo stage */
    bay_px_motion_trail(fb, player, style, frame, guarding, striking,
                        ground_mode, style_id);          /* strike sweep */
    bay_px_rasterize(fb, v, player, style_id);           /* the fighter */

    if (backend == BAY_BACKEND_KITTY) bay_kitty_emit(fb, x, y);
    else                              bay_sixel_emit(fb, x, y);
    free(fb);
}

#endif /* BAYEISHUUI_PIXELS_H */
