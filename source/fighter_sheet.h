#ifndef BAYEISHUUI_FIGHTER_SHEET_H
#define BAYEISHUUI_FIGHTER_SHEET_H

/*
 * fighter_sheet.h -- Bayeishuui SLeeLa Character Sheet reader / interpreter /
 * disposer for the native C and C++ games.
 *
 * A character sheet is a SLeeLa class file (fighters/<name>.sleela) whose body
 * holds flat `type name = value;` field initialisers. This header turns such a
 * sheet into an in-game BayFighter the combat loop can use:
 *
 *   read      bay_sheet_read(path)           file -> heap text buffer
 *   interpret bay_fighter_from_source(text)  text -> BayFighter (by value)
 *             bay_fighter_load(path, out)     convenience: read + interpret
 *   dispose   bay_sheet_free(buffer)          release the text buffer
 *             bay_fighter_dispose(fighter)    release any owned fields
 *
 * The sheet is treated as perfect, well-formed input: fields are parsed
 * leniently, unknown fields are ignored, and any field not present keeps the
 * engine default from bay_fighter_default(). The reader never executes SLeeLa;
 * it extracts the field initialisers (the stats are known constants).
 *
 * Shared by bayeishuui.c and bayeishuui.cpp, matching the existing shared-header
 * style of bayeishuui_sprites.h / bayeishuui_ai.h. C11 and C++17 clean.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Field capacities. Strings are fixed-width so a BayFighter is a plain value
 * type (no heap) unless the caller opts into the owned-copy helpers. */
#define BAY_FIGHTER_NAME_MAX   48
#define BAY_FIGHTER_RANK_MAX   32
#define BAY_FIGHTER_STYLE_MAX  48
#define BAY_FIGHTER_TACTIC_MAX 128

/* The interpreted fighter the native game consumes. Numeric units match the
 * sheet field comments: reach in hundredths of a metre, paceCentis in
 * hundredths (100 == 1.00x), windup in milliseconds, tendencies 0..100. */
typedef struct BayFighter {
    char name[BAY_FIGHTER_NAME_MAX];
    char rankTitle[BAY_FIGHTER_RANK_MAX];
    char style[BAY_FIGHTER_STYLE_MAX];
    char tactic[BAY_FIGHTER_TACTIC_MAX];
    int  rank;         /* 1..15 Fighter Guild position              */
    int  health;       /* starting HP                               */
    int  stamina;      /* starting stamina                          */
    int  reach;        /* hundredths of a metre                     */
    int  baseDamage;   /* base strike damage before variance        */
    int  windupMs;     /* telegraph time before a strike lands (ms) */
    int  paceCentis;   /* decision cadence, hundredths of 1.0x      */
    int  aggression;   /* 0..100 closing/commitment tendency        */
    int  lateral;      /* 0..100 circling/angle-change tendency     */
    int  colorR, colorG, colorB; /* terminal sprite accent, 0..255  */
    int  valid;        /* 1 if a Fighter class body was interpreted */
} BayFighter;

/* ---- helpers --------------------------------------------------------- */

static void bay_sheet_copy_field(char *dst, size_t cap, const char *src, size_t n) {
    if (cap == 0) return;
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/* A sheet with no fields still produces a usable, neutral fighter. */
static BayFighter bay_fighter_default(void) {
    BayFighter f;
    memset(&f, 0, sizeof f);
    bay_sheet_copy_field(f.name, sizeof f.name, "Unknown Fighter", 15);
    bay_sheet_copy_field(f.rankTitle, sizeof f.rankTitle, "Initiate", 8);
    bay_sheet_copy_field(f.style, sizeof f.style, "Muay Thai Fundamentals", 22);
    bay_sheet_copy_field(f.tactic, sizeof f.tactic, "Balanced stance.", 16);
    f.rank = 1; f.health = 100; f.stamina = 100;
    f.reach = 210; f.baseDamage = 6; f.windupMs = 400; f.paceCentis = 100;
    f.aggression = 40; f.lateral = 20;
    f.colorR = 200; f.colorG = 200; f.colorB = 255;
    f.valid = 0;
    return f;
}

/* ---- READER: file -> heap text buffer -------------------------------- */

/* Read an entire character-sheet file into a NUL-terminated heap buffer.
 * Returns NULL on open/allocation/read failure. Caller frees with
 * bay_sheet_free(). */
static char *bay_sheet_read(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return NULL; }
    long end = ftell(fp);
    if (end < 0) { fclose(fp); return NULL; }
    if (fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); return NULL; }
    size_t size = (size_t)end;
    char *buf = (char *)malloc(size + 1);
    if (!buf) { fclose(fp); return NULL; }
    size_t got = fread(buf, 1, size, fp);
    fclose(fp);
    buf[got] = '\0';
    return buf;
}

static void bay_sheet_free(char *buffer) {
    free(buffer);
}

/* ---- INTERPRETER: text -> BayFighter --------------------------------- */

/* Locate `key` as a field name in the sheet source and copy its initialiser
 * value. Matches `... key = <value> ;` where key is a whole identifier.
 * Returns 1 and writes [value_begin, *value_len) on success, else 0. */
static int bay_sheet_find_value(const char *src, const char *key,
                                const char **value_begin, size_t *value_len) {
    size_t klen = strlen(key);
    const char *p = src;
    while ((p = strstr(p, key)) != NULL) {
        /* whole-identifier match: boundaries on both sides */
        int left_ok  = (p == src) || (!isalnum((unsigned char)p[-1]) && p[-1] != '_');
        const char *after = p + klen;
        int right_ok = !isalnum((unsigned char)*after) && *after != '_';
        if (left_ok && right_ok) {
            const char *q = after;
            while (*q == ' ' || *q == '\t') q++;
            if (*q == '=') {
                q++;
                while (*q == ' ' || *q == '\t') q++;
                const char *vb; size_t vl;
                if (*q == '"') {               /* quoted string value */
                    q++;
                    vb = q;
                    while (*q && *q != '"') q++;
                    vl = (size_t)(q - vb);
                } else {                        /* bare numeric/token value */
                    vb = q;
                    while (*q && *q != ';' && *q != '\n' && *q != '\r' &&
                           *q != ' ' && *q != '\t') q++;
                    vl = (size_t)(q - vb);
                }
                *value_begin = vb;
                *value_len = vl;
                return 1;
            }
        }
        p = after;
    }
    return 0;
}

static void bay_sheet_read_str(const char *src, const char *key,
                               char *dst, size_t cap) {
    const char *vb; size_t vl;
    if (bay_sheet_find_value(src, key, &vb, &vl))
        bay_sheet_copy_field(dst, cap, vb, vl);
}

static void bay_sheet_read_int(const char *src, const char *key, int *dst) {
    const char *vb; size_t vl;
    if (bay_sheet_find_value(src, key, &vb, &vl) && vl > 0) {
        char tmp[32];
        bay_sheet_copy_field(tmp, sizeof tmp, vb, vl);
        *dst = (int)strtol(tmp, NULL, 10);
    }
}

/* Interpret a character-sheet source string into a BayFighter. Only the body
 * of a `class Fighter { ... }` is considered; fields outside it are ignored.
 * Missing fields keep bay_fighter_default() values. */
static BayFighter bay_fighter_from_source(const char *src) {
    BayFighter f = bay_fighter_default();
    if (!src) return f;

    /* Scope parsing to the Fighter class body when present. */
    const char *scope = strstr(src, "class Fighter");
    const char *body = scope ? strchr(scope, '{') : src;
    if (!body) body = src;

    bay_sheet_read_str(body, "name",       f.name,      sizeof f.name);
    bay_sheet_read_str(body, "rankTitle",  f.rankTitle, sizeof f.rankTitle);
    bay_sheet_read_str(body, "style",      f.style,     sizeof f.style);
    bay_sheet_read_str(body, "tactic",     f.tactic,    sizeof f.tactic);
    bay_sheet_read_int(body, "rank",       &f.rank);
    bay_sheet_read_int(body, "health",     &f.health);
    bay_sheet_read_int(body, "stamina",    &f.stamina);
    bay_sheet_read_int(body, "reach",      &f.reach);
    bay_sheet_read_int(body, "baseDamage", &f.baseDamage);
    bay_sheet_read_int(body, "windupMs",   &f.windupMs);
    bay_sheet_read_int(body, "paceCentis", &f.paceCentis);
    bay_sheet_read_int(body, "aggression", &f.aggression);
    bay_sheet_read_int(body, "lateral",    &f.lateral);
    bay_sheet_read_int(body, "colorR",     &f.colorR);
    bay_sheet_read_int(body, "colorG",     &f.colorG);
    bay_sheet_read_int(body, "colorB",     &f.colorB);

    f.valid = (scope != NULL);
    return f;
}

/* Convenience: read a file and interpret it in one call. Returns 1 on success
 * (a sheet file was read and interpreted), 0 if the file could not be read
 * (out is then left as bay_fighter_default()). */
static int bay_fighter_load(const char *path, BayFighter *out) {
    char *text = bay_sheet_read(path);
    if (!text) { if (out) *out = bay_fighter_default(); return 0; }
    if (out) *out = bay_fighter_from_source(text);
    bay_sheet_free(text);
    return 1;
}

/* ---- DISPOSER -------------------------------------------------------- */

/* Reset an interpreted fighter to a clean, inert state. BayFighter owns no
 * heap (its strings are inline), so disposal is a scrub + invalidate; keeping
 * an explicit disposer gives the game one clear lifecycle contract and a hook
 * if owned fields are added later. */
static void bay_fighter_dispose(BayFighter *fighter) {
    if (!fighter) return;
    memset(fighter, 0, sizeof *fighter);
    fighter->valid = 0;
}

#endif /* BAYEISHUUI_FIGHTER_SHEET_H */
