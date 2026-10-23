/*
 * fighter_sheet_demo.c -- C demonstration of the Bayeishuui SLeeLa character
 * sheet reader / interpreter / disposer. Reads one or more .sleela sheets,
 * interprets each into a BayFighter, prints the interpreted stats, then
 * disposes. Build: make build-sheet-c ; run: ./build/bayeishuui-sheet-c <sheet>...
 */
#include <stdio.h>
#include "fighter_sheet.h"

static void print_fighter(const char *path, const BayFighter *f) {
    printf("=== %s ===\n", path);
    printf("  name       : %s\n", f->name);
    printf("  rank       : %d (%s)\n", f->rank, f->rankTitle);
    printf("  style      : %s\n", f->style);
    printf("  tactic     : %s\n", f->tactic);
    printf("  health     : %d\n", f->health);
    printf("  stamina    : %d\n", f->stamina);
    printf("  reach      : %.2f m\n", f->reach / 100.0);
    printf("  baseDamage : %d\n", f->baseDamage);
    printf("  windup     : %d ms\n", f->windupMs);
    printf("  pace       : %.2fx\n", f->paceCentis / 100.0);
    printf("  aggression : %d/100\n", f->aggression);
    printf("  lateral    : %d/100\n", f->lateral);
    printf("  colour     : (%d,%d,%d)\n", f->colorR, f->colorG, f->colorB);
    printf("  interpreted: %s\n", f->valid ? "yes" : "no (defaults)");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <character-sheet.sleela> [more.sleela ...]\n", argv[0]);
        return 2;
    }
    for (int i = 1; i < argc; ++i) {
        BayFighter f;
        if (!bay_fighter_load(argv[i], &f)) {
            fprintf(stderr, "could not read sheet: %s\n", argv[i]);
            continue;
        }
        print_fighter(argv[i], &f);
        bay_fighter_dispose(&f);
        if (i + 1 < argc) printf("\n");
    }
    return 0;
}
