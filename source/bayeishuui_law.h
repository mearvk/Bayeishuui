#ifndef BAYEISHUUI_LAW_H
#define BAYEISHUUI_LAW_H

/*
 * BAYEISHUUI — Military Martial Arts Law (MMAL).
 *
 * Header-only rules module shared by the native C11 and C++17 games. It holds
 * the pure, deterministic decision logic for joint-break injuries so it can be
 * reasoned about (and tested) apart from rendering and input.
 *
 *   Article 1  A fighter may attempt an ARM LOCK (key B) at clinch range, or
 *              from ground control (2+). A successful lock breaks one arm.
 *   Article 2  A broken arm NEVER stops the bout. The injured fighter fights on.
 *   Article 3  Each arm breaks once per bout; a fighter can lose at most both.
 *   Article 4  Injury has a tactical price: punches/ground-and-pound weaken,
 *              the guard leaks, and the rival's tempo slows. Legs are unharmed,
 *              so teeps and round kicks stay at full strength.
 *   Article 5  Between bouts the corner sets the arms: injuries clear.
 *
 * Injury state lives in bay_injury_mask[] (defined in bayeishuui_sprites.h so
 * the renderers can pose a limp arm): index 0 = rival, index 1 = player. Bit 1
 * is the left arm, bit 2 the right arm.
 */
#include <stdio.h>
#include <stdlib.h>
#include "bayeishuui_sprites.h"

#define BAY_LAW_CLINCH_RANGE   2.0   /* max gap (m) for a stand-up arm lock  */
#define BAY_LAW_BREAK_STAMINA  20    /* stamina cost of an arm-lock attempt  */
#define BAY_LAW_BREAK_DAMAGE   14    /* HP lost to the bone-break itself     */

static inline int bay_law_arms_broken(int mask) { return ((mask & 1) ? 1 : 0) + ((mask & 2) ? 1 : 0); }

/* Both arms broken: no punches or elbows, only kicks. */
static inline int bay_law_can_punch(int mask) { return (mask & 3) != 3; }

/* Multiplier on arm-delivered damage (punches, elbows, ground-and-pound). */
static inline double bay_law_damage_scale(int mask) {
    int n = bay_law_arms_broken(mask);
    return n == 0 ? 1.0 : (n == 1 ? 0.60 : 0.35);
}

/* Multiplier on damage that leaks through a guard (1, 2 or 3x). */
static inline int bay_law_guard_penalty(int mask) { return 1 + bay_law_arms_broken(mask); }

/* Multiplier on the injured rival's attack interval (slower when hurt). */
static inline double bay_law_enemy_tempo(int mask) {
    int n = bay_law_arms_broken(mask);
    return n == 0 ? 1.0 : (n == 1 ? 1.30 : 1.70);
}

/* Percent chance (0..95) that your arm-lock attempt breaks an arm. */
static inline int bay_law_break_chance(int rank, int ground_control, int ground_mode, double enemy_stamina) {
    int c = ground_mode ? 40 + 8 * ground_control - rank : 34 - rank;
    if (enemy_stamina < 30.0) c += 10;          /* a tired fighter cannot resist */
    if (c < 12) c = 12;
    if (c > 95) c = 95;
    return c;
}

/* Percent chance that a landed, unguarded rival strike also breaks your arm. */
static inline int bay_law_enemy_break_chance(int rank, double player_stamina) {
    if (rank < 3) return 0;                     /* early opponents are not yet that dangerous */
    int c = 1 + rank * 2 / 3;
    if (rank == 7) c += 6;                      /* Clinch Controller */
    if (player_stamina < 25.0) c += 6;          /* exhaustion makes you easy to lock */
    return c > 20 ? 20 : c;
}

/* Choose an arm that is not yet broken. Returns 1 (left), 2 (right), 0 (none). */
static inline int bay_law_pick_arm(int mask, int rnd) {
    int free_arms = (~mask) & 3;
    if (!free_arms) return 0;
    if (free_arms == 3) return (rnd & 1) ? 2 : 1;
    return free_arms;
}

static inline const char *bay_law_arm_name(int arm) { return arm == 1 ? "left" : (arm == 2 ? "right" : "no"); }

/*
 * Perfect Fighter extension. Returns 'b' (attempt a lock), 'w' (close in to
 * lock range) or '\0' (defer to the normal AI). Never acts while the rival is
 * winding up a strike, so it keeps the existing defensive priorities.
 */
static inline char bay_law_ai_action(double hp, double stamina, double gap, int lateral,
                                     int ground_mode, int ground_control,
                                     double enemy_hp, int enemy_mask,
                                     double enemy_wind, double t, double cooldown) {
    if (enemy_mask == 3 || enemy_hp <= 22.0 || stamina < BAY_LAW_BREAK_STAMINA + 4) return '\0';
    if (enemy_wind > t && enemy_wind - t < 0.50) return '\0';
    if (t < cooldown) return '\0';
    if (ground_mode) return ground_control >= 2 ? 'b' : '\0';
    if (lateral > 2 || lateral < -2) return '\0';
    if (gap <= BAY_LAW_CLINCH_RANGE) return 'b';
    if (enemy_mask == 0 && gap <= 3.0 && stamina >= 40.0 && hp >= 50.0) return 'w';
    return '\0';
}

/* HUD status line at terminal row `row`: arm condition for both fighters. */
static inline void bay_law_draw_status(int row) {
    const char *ok = "\033[38;2;95;235;155mOK\033[0m";
    const char *bad = "\033[38;2;255;90;105mBROKEN\033[0m";
    printf("\033[%d;1H\033[38;2;190;200;215m MMAL ARMS  YOU L:%s R:%s  RIVAL L:%s R:%s  \033[38;2;255;205;115m[B] arm lock\033[0m",
           row,
           (bay_injury_mask[1] & 1) ? bad : ok, (bay_injury_mask[1] & 2) ? bad : ok,
           (bay_injury_mask[0] & 1) ? bad : ok, (bay_injury_mask[0] & 2) ? bad : ok);
}

#endif /* BAYEISHUUI_LAW_H */
