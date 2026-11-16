/*
 * bayeishuui-pixeldemo — exercise the pixel-accurate fighter renderer.
 *
 * Usage:
 *   bayeishuui-pixeldemo [options]
 *     --backend=auto|kitty|sixel|glyph   pick transport (default: auto-detect)
 *     --style=N        opponent style 0..14 (default 0)
 *     --frame=N        animation frame 0..243 (default 42)
 *     --player         render the player-blue fighter instead of an opponent
 *     --guard | --strike | --ground   pose modifiers
 *     --hair=N         hair style 0..7 (Buzz Cut, Lover, Topknot, Man, Afro, Punk, Braid, Mane)
 *     --hair-level=N   hair upgrade tier 0 natural, 1 dyed, 2 glow
 *     --joe            render Joe Biden (White Belt wildcard) as the opponent
 *     --smoke          with --joe: show his lit cigarette and smoke
 *     --white-hat      with --joe: the career white hat (after his first arm snap)
 *     --broken=N       Military Martial Arts Law injury: 1 left arm, 2 right, 3 both
 *     --ppm=FILE       also write the raw RGBA frame as a PPM image (for
 *                      verifying pixel output without a graphical terminal)
 *     --frames=N       animate N frames in place (Kitty/Sixel terminals)
 *
 * The PPM dump makes the renderer testable anywhere: the exact pixels the
 * terminal would receive are written to a standard image file.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "bayeishuui_pixels.h"

static const char *backend_name(BayBackend b) {
    switch (b) {
        case BAY_BACKEND_KITTY: return "kitty";
        case BAY_BACKEND_SIXEL: return "sixel";
        case BAY_BACKEND_GLYPH: return "glyph";
        default:                return "unknown";
    }
}

static void write_ppm(const char *path, int player, int style, int frame,
                      int guard, int strike, int ground) {
    int v[24], style_id = style < 0 ? 0 : style % 15;
    bay_sprite_skeleton(v, player, style, frame, guard, strike, ground);
    BayRGBA *fb = calloc((size_t)(BAY_PX_W * BAY_PX_H), sizeof(BayRGBA));
    if (!fb) return;
    /* Use the exact same compositing the game does: dojo backdrop, then the
     * strike motion trail, then the fighter — so the PPM is a faithful
     * preview of on-screen pixels. */
    bay_px_backdrop(fb);
    bay_px_motion_trail(fb, player, style, frame, guard, strike, ground, style_id);
    bay_px_rasterize(fb, v, player, style_id);
    FILE *f = fopen(path, "wb");
    if (!f) { free(fb); return; }
    fprintf(f, "P6\n%d %d\n255\n", BAY_PX_W, BAY_PX_H);
    for (int i = 0; i < BAY_PX_W * BAY_PX_H; ++i) {
        fputc(fb[i].r, f); fputc(fb[i].g, f); fputc(fb[i].b, f);
    }
    fclose(f);
    free(fb);
    fprintf(stderr, "Wrote %s (%dx%d, dojo backdrop + fighter)\n",
            path, BAY_PX_W, BAY_PX_H);
}

int main(int argc, char **argv) {
    BayBackend backend = BAY_BACKEND_UNKNOWN;
    int style = 0, frame = 42, player = 0, guard = 0, strike = 0, ground = 0;
    int frames = 1, broken = 0, hair = 0, hair_lvl = 0, joe = 0, smoke = 0, white_hat = 0;
    const char *ppm = NULL, *forced = NULL;

    for (int i = 1; i < argc; ++i) {
        if      (!strncmp(argv[i], "--backend=", 10)) forced = argv[i] + 10;
        else if (!strncmp(argv[i], "--style=", 8))    style  = atoi(argv[i] + 8);
        else if (!strncmp(argv[i], "--frame=", 8))    frame  = atoi(argv[i] + 8);
        else if (!strncmp(argv[i], "--frames=", 9))   frames = atoi(argv[i] + 9);
        else if (!strncmp(argv[i], "--ppm=", 6))      ppm    = argv[i] + 6;
        else if (!strcmp(argv[i], "--player"))        player = 1;
        else if (!strcmp(argv[i], "--guard"))         guard  = 1;
        else if (!strcmp(argv[i], "--strike"))        strike = 1;
        else if (!strcmp(argv[i], "--ground"))        ground = 1;
        else if (!strcmp(argv[i], "--joe"))           joe    = 1;
        else if (!strcmp(argv[i], "--smoke"))         smoke  = 1;
        else if (!strcmp(argv[i], "--white-hat"))     white_hat = 1;
        else if (!strncmp(argv[i], "--broken=", 9))   broken = atoi(argv[i] + 9);
        else if (!strncmp(argv[i], "--hair=", 7))     hair = atoi(argv[i] + 7);
        else if (!strncmp(argv[i], "--hair-level=", 13)) hair_lvl = atoi(argv[i] + 13);
        else if (!strcmp(argv[i], "--help")) {
            fprintf(stderr,
              "bayeishuui-pixeldemo [--backend=auto|kitty|sixel|glyph] "
              "[--style=N] [--frame=N] [--frames=N] [--player] "
              "[--guard|--strike|--ground] [--joe [--smoke] [--white-hat]] [--broken=1|2|3] [--hair=0..7] [--hair-level=0..2] [--ppm=FILE]\n");
            return 0;
        }
    }

    if (forced) {
        if      (!strcmp(forced, "kitty")) backend = BAY_BACKEND_KITTY;
        else if (!strcmp(forced, "sixel")) backend = BAY_BACKEND_SIXEL;
        else if (!strcmp(forced, "glyph")) backend = BAY_BACKEND_GLYPH;
        else backend = bay_detect_backend();   /* "auto" or anything else */
    } else {
        backend = bay_detect_backend();
    }

    if (joe && !player) { bay_joe.active = 1; bay_joe.smoking = smoke; bay_joe_white_hat = white_hat; }
    bay_injury_mask[player ? 1 : 0] = (unsigned char)(broken & 3);
    bay_hair_sel[player ? 1 : 0] = (hair < 0 || hair >= BAY_HAIR_COUNT) ? 0 : hair;
    bay_hair_lvl[player ? 1 : 0] = (hair_lvl < 0 || hair_lvl > BAY_HAIR_MAX_LEVEL) ? 0 : hair_lvl;

    if (ppm) write_ppm(ppm, player, style, frame, guard, strike, ground);

    fprintf(stderr, "Renderer backend: %s  (override with BAYEISHUUI_RENDERER "
                    "or --backend=)\n", backend_name(backend));

    if (!isatty(STDOUT_FILENO) && backend != BAY_BACKEND_GLYPH && !ppm) {
        fprintf(stderr, "stdout is not a terminal; use --ppm=FILE to capture "
                        "pixels, or --backend=glyph for text.\n");
        return 0;
    }

    struct timespec nap = {0, 60 * 1000000L};
    printf("\033[2J");
    for (int n = 0; n < (frames < 1 ? 1 : frames); ++n) {
        bay_pixel_draw_backend(backend, 3, 4, player, style, frame + n,
                               guard, strike, ground);
        fflush(stdout);
        if (frames > 1) nanosleep(&nap, NULL);
    }
    printf("\033[%d;1H\n", BAY_PX_H / 20 + 4);
    return 0;
}
