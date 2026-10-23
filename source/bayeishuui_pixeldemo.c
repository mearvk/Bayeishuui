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
    bay_px_rasterize(fb, v, player, style_id);
    FILE *f = fopen(path, "wb");
    if (!f) { free(fb); return; }
    /* composite onto a dark dojo background so the PPM is viewable */
    fprintf(f, "P6\n%d %d\n255\n", BAY_PX_W, BAY_PX_H);
    for (int i = 0; i < BAY_PX_W * BAY_PX_H; ++i) {
        unsigned a = fb[i].a;
        unsigned char bgr = 12, bgg = 16, bgb = 28;
        unsigned char r = (unsigned char)((fb[i].r * a + bgr * (255-a)) / 255);
        unsigned char g = (unsigned char)((fb[i].g * a + bgg * (255-a)) / 255);
        unsigned char b = (unsigned char)((fb[i].b * a + bgb * (255-a)) / 255);
        fputc(r, f); fputc(g, f); fputc(b, f);
    }
    fclose(f);
    free(fb);
    fprintf(stderr, "Wrote %s (%dx%d RGBA composited over dojo background)\n",
            path, BAY_PX_W, BAY_PX_H);
}

int main(int argc, char **argv) {
    BayBackend backend = BAY_BACKEND_UNKNOWN;
    int style = 0, frame = 42, player = 0, guard = 0, strike = 0, ground = 0;
    int frames = 1;
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
        else if (!strcmp(argv[i], "--help")) {
            fprintf(stderr,
              "bayeishuui-pixeldemo [--backend=auto|kitty|sixel|glyph] "
              "[--style=N] [--frame=N] [--frames=N] [--player] "
              "[--guard|--strike|--ground] [--ppm=FILE]\n");
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
        bay_pixel_draw_backend(backend, 3, 2, player, style, frame + n,
                               guard, strike, ground);
        fflush(stdout);
        if (frames > 1) nanosleep(&nap, NULL);
    }
    printf("\033[%d;1H\n", BAY_PX_H / 20 + 4);
    return 0;
}
