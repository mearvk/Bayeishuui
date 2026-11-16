#ifndef BAYEISHUUI_JOE_H
#define BAYEISHUUI_JOE_H

/*
 * BAYEISHUUI — "Joe Biden", the White Belt wildcard.
 *
 * A fictional, parody guest fighter shared by the C11 and C++17 games. This
 * header holds the pure, deterministic rules and state (apart from rand()) so
 * they can be reasoned about apart from rendering and input; the sprite and
 * pixel renderers read the `bay_joe` state below to dress him.
 *
 *   Entry     In 1 of every BAY_JOE_ENTRY_ODDS (4) bouts Joe takes the rival
 *             slot ("the first fighter") in place of that bout's Guild
 *             opponent. He fights with White Belt (rank 1) tactics.
 *   Hit points  1500 (the Guild standard is 100). The rival HP bar stays a
 *             percentage, so every point of damage dealt to Joe is converted
 *             with bay_joe_dmg().
 *   Arm snap  A Military Martial Arts Law arm break costs Joe 750 HP instead
 *             of the usual BAY_LAW_BREAK_DAMAGE.
 *   Cigarette After an arm snap he lights a cigarette for 1d6 + 8 seconds.
 *             When it burns out there is a 50% chance he falls over, which
 *             puts the bout into the ground game (ground-and-pound). Once he
 *             is down there is a 1 in 4 chance of the "You beftta!" bubble.
 *   Deal      Typing "zz" (two z presses within 0.65 s) in the FIRST TWO SECONDS
 *             of a Joe bout opens "A Deal for Joe": ribs and sauce or steak and
 *             potatoes, if you agree that Joe is main and kosher. Answer y or n
 *             (the bout is paused while the question is open). Either way Joe
 *             suffers a fractured arm (the usual 750 HP, even when an arm is
 *             already snapped); on n he also loses BAY_JOE_DEAL_NO_HP (900) HP.
 *             Joe shouts "Bark" from his dialog box, then it is back to the
 *             fight. One deal per bout.
 *   Hat       Red until his first arm snaps. From that moment it is WHITE for
 *             the rest of his career: the flag is kept in the saved profile
 *             (joe_white_hat) and mirrored in bay_joe_white_hat below.
 *   Look      White skin, red cowboy hat (white after the first snap), white
 *             belt, 14% taller than every other fighter, comic speech bubbles.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>

#define BAY_JOE_NAME           "Joe Biden"
#define BAY_JOE_RANK_TITLE     "White Belt"
#define BAY_JOE_STYLE          "White Belt Brawler (talks a lot, in a red cowboy hat)"
#define BAY_JOE_TACTIC         "Soaks up punishment; lights up after an arm snap. Type zz in 2s for the Deal."
#define BAY_JOE_MAX_HP         1500   /* guild fighters have 100              */
#define BAY_JOE_ARM_BREAK_HP   750    /* HP deducted by one snapped arm       */
#ifndef BAY_JOE_ENTRY_ODDS
#define BAY_JOE_ENTRY_ODDS     4      /* 1 in 4 bouts (overridable for tests) */
#endif
#define BAY_JOE_HEIGHT_PCT     114    /* 14% taller than the rest             */
#define BAY_JOE_SMOKE_BASE_S   8      /* cigarette lasts 1d6 + 8 seconds      */
#define BAY_JOE_FALL_PCT       50     /* chance he falls when it burns out    */
#define BAY_JOE_TAUNT_ODDS     4      /* 1 in 4 falls get "You beftta!"       */
#define BAY_JOE_BUBBLE_SECS    3.5
#define BAY_JOE_DEAL_WINDOW_S  2.0   /* the Deal must be opened this soon    */
#define BAY_JOE_DEAL_ZZ_S      0.65  /* max gap between the two z presses    */
#define BAY_JOE_DEAL_NO_HP     900   /* extra HP lost when the answer is n   */
#define BAY_JOE_BARK           "Bark"
#define BAY_JOE_EFFECTIVE_RANK 1      /* White Belt tactics and arm-lock odds */

typedef struct BayJoe {
    int    active;        /* Joe is the rival in the current bout              */
    int    snapped;       /* an arm has been snapped this bout                 */
    int    fallen;        /* he has fallen over this bout                      */
    int    smoking;       /* cigarette currently lit                           */
    double smoke_until;   /* monotonic seconds when the cigarette burns out    */
    double t0;            /* time of the first tick of this bout (0 = not yet) */
    int    deal_done;     /* the Deal for Joe was already used this bout       */
    int    bubble_on;     /* a speech bubble is showing                        */
    double bubble_until;
    char   bubble[96];
} BayJoe;

/* Zero-initialised: Joe is not in the bout until bay_joe_start_bout() says so. */
static BayJoe bay_joe;

/* Career flag (survives bouts, unlike bay_joe): hat is white once set. The game
 * loads it from the profile at start and saves it when it first becomes 1. */
static int bay_joe_white_hat = 0;

/* Pool of arm-snap lines. The first is the canonical one. */
static inline const char *bay_joe_snap_line(int r) {
    static const char *lines[] = {
        "Ow, You owe me Meanies!",
        "Ow! You owe me, ya meanie!",
        "OW! That's gonna cost ya, Meanies!",
        "Ow, folks! You owe me Meanies!",
        "My arm! Meanies owe me big!"
    };
    return lines[(unsigned)r % (sizeof lines / sizeof lines[0])];
}

static inline void bay_joe_say(const char *text, double now) {
    snprintf(bay_joe.bubble, sizeof bay_joe.bubble, "%s", text);
    bay_joe.bubble_on = 1;
    bay_joe.bubble_until = now + BAY_JOE_BUBBLE_SECS;
}

/* Name shown for the current rival (Joe, or the Guild opponent passed in). */
static inline const char *bay_joe_name_or(const char *guild_name) {
    return bay_joe.active ? BAY_JOE_NAME : guild_name;
}

/* Rank used for the rival's tactics and for the player's arm-lock odds. */
static inline int bay_joe_rank_or(int guild_rank) {
    return bay_joe.active ? BAY_JOE_EFFECTIVE_RANK : guild_rank;
}

/* Start a bout: reset all Joe state, then roll 1 in 4 for his entry. */
static inline void bay_joe_start_bout(void) {
    memset(&bay_joe, 0, sizeof bay_joe);
    bay_joe.active = (rand() % BAY_JOE_ENTRY_ODDS) == 0;
}

/* Convert HP of damage into rival-bar percentage points (the bar is 0..100). */
static inline double bay_joe_dmg(double hp_damage) {
    return bay_joe.active ? hp_damage * 100.0 / BAY_JOE_MAX_HP : hp_damage;
}

/* Rival HP in real hit points for display (bar percent -> Joe's 1500 scale). */
static inline int bay_joe_hp_now(double bar_percent) {
    if (bar_percent < 0) bar_percent = 0;
    return (int)(bar_percent * BAY_JOE_MAX_HP / 100.0 + 0.5);
}

/* HP removed by an arm snap, expressed in rival-bar percentage points. */
static inline double bay_joe_snap_cost(double guild_cost) {
    return bay_joe.active ? bay_joe_dmg(BAY_JOE_ARM_BREAK_HP) : guild_cost;
}

/* Called when an arm of the rival breaks. Lights the cigarette (1d6 + 8 s). */
static inline void bay_joe_on_arm_snap(double now) {
    if (!bay_joe.active) return;
    bay_joe_white_hat = 1;
    bay_joe_say(bay_joe_snap_line(rand()), now);
    if (bay_joe.snapped || bay_joe.fallen) return;      /* one cigarette per bout */
    bay_joe.snapped = 1;
    bay_joe.smoking = 1;
    bay_joe.smoke_until = now + BAY_JOE_SMOKE_BASE_S + (1 + rand() % 6);
}

/*
 * Advance Joe's timers. Returns 1 on the tick he falls over (the caller then
 * starts the ground game), otherwise 0.
 */
static inline int bay_joe_tick(double now) {
    int fell = 0;
    if (!bay_joe.active) return 0;
    if (bay_joe.t0 == 0) bay_joe.t0 = now;
    if (bay_joe.bubble_on && now >= bay_joe.bubble_until) bay_joe.bubble_on = 0;
    if (bay_joe.smoking && now >= bay_joe.smoke_until) {
        bay_joe.smoking = 0;
        if ((rand() % 100) < BAY_JOE_FALL_PCT) {
            bay_joe.fallen = 1;
            fell = 1;
            if ((rand() % BAY_JOE_TAUNT_ODDS) == 0) bay_joe_say("You beftta!", now);
        }
    }
    return fell;
}

/* Push Joe's timers forward after the game was paused (Barber Shop). */
static inline void bay_joe_shift(double seconds) {
    if (bay_joe.t0 > 0) bay_joe.t0 += seconds;
    if (bay_joe.smoking) bay_joe.smoke_until += seconds;
    if (bay_joe.bubble_on) bay_joe.bubble_until += seconds;
}

/* ------------------------------------------------------------ the Deal */
/* Result codes of bay_joe_deal_key(). */
enum { BAY_JOE_DEAL_NONE = 0, BAY_JOE_DEAL_OPEN = 1, BAY_JOE_DEAL_TOO_LATE = 2,
       BAY_JOE_DEAL_NO_JOE = 3, BAY_JOE_DEAL_USED = 4 };

/* Feed every 'z' press here. The second z within BAY_JOE_DEAL_ZZ_S of the
 * first completes "zz". Returns BAY_JOE_DEAL_OPEN when the Deal should be
 * offered now, another BAY_JOE_DEAL_* code explaining why not, or
 * BAY_JOE_DEAL_NONE for the first z. */
static inline int bay_joe_deal_key(double now) {
    static double last_z = -10.0;
    int second = (now - last_z) <= BAY_JOE_DEAL_ZZ_S;
    last_z = second ? -10.0 : now;
    if (!second) return BAY_JOE_DEAL_NONE;
    if (!bay_joe.active) return BAY_JOE_DEAL_NO_JOE;
    if (bay_joe.deal_done) return BAY_JOE_DEAL_USED;
    if (bay_joe.t0 > 0 && now - bay_joe.t0 > BAY_JOE_DEAL_WINDOW_S) return BAY_JOE_DEAL_TOO_LATE;
    return BAY_JOE_DEAL_OPEN;
}

/*
 * Full-screen y/n question (like the Barber Shop, the bout is paused). Returns
 * 1 for y, 0 for n, -1 if alive() reports the program is shutting down.
 */
static inline int bay_joe_deal_prompt(int (*alive)(void)) {
    printf("\033[2J\033[H\033[?25l");
    printf("\033[1;38;2;130;205;255m  BAYEISHUUI  /  A DEAL FOR JOE\033[0m\n\n");
    printf("\033[38;2;255;215;120m  Joe Biden leans in and offers you a meal:\033[0m\n\n");
    printf("\033[1;38;2;255;238;225m      RIBS AND SAUCE\033[0m   \033[38;2;190;200;215m-or-\033[0m   \033[1;38;2;255;238;225mSTEAK AND POTATOES\033[0m\n\n");
    printf("\033[38;2;220;225;235m  ...if you agree that Joe is main and kosher.\033[0m\n\n");
    printf("\033[38;2;95;235;155m  [y] Yes, Joe is main and kosher\033[0m\n");
    printf("\033[38;2;255;150;110m  [n] No  (Joe loses %d more hit points)\033[0m\n\n", BAY_JOE_DEAL_NO_HP);
    printf("\033[38;2;155;190;220m  Either way his arm is fractured. Press y or n.\033[0m\n");
    fflush(stdout);
    for (;;) {
        if (!alive()) return -1;
        fd_set f; struct timeval tv = {0, 100000};
        FD_ZERO(&f); FD_SET(STDIN_FILENO, &f);
        if (select(STDIN_FILENO + 1, &f, NULL, NULL, &tv) > 0) {
            char c = 0;
            if (read(STDIN_FILENO, &c, 1) == 1) {
                if (c == 'y' || c == 'Y') return 1;
                if (c == 'n' || c == 'N') return 0;
            }
        }
    }
}

/* HP (in rival-bar percentage points) Joe loses for an answer: the arm fracture
 * (always, even if an arm is already snapped) plus the 900 on "no". */
static inline double bay_joe_deal_cost(int agreed) {
    return bay_joe_dmg(BAY_JOE_ARM_BREAK_HP) + (agreed ? 0.0 : bay_joe_dmg(BAY_JOE_DEAL_NO_HP));
}

/* Status line for a deal key result that did not open the Deal. */
static inline const char *bay_joe_deal_refusal(int code) {
    return code == BAY_JOE_DEAL_NO_JOE   ? "zz: the Deal is only for Joe - he is not in this bout." :
           code == BAY_JOE_DEAL_USED     ? "Joe already made his Deal this bout." :
           code == BAY_JOE_DEAL_TOO_LATE ? "Too late - the Deal for Joe is only open in the first 2 seconds." : "";
}

/*
 * Comic speech bubble drawn beside Joe. (fig_col, fig_row) is the figure's
 * top-left terminal cell, fig_cols its width in terminal columns, w the
 * terminal width. It sits to the right of the figure, or to the left when
 * there is no room, so it never covers the pixel image.
 */
static inline void bay_joe_draw_bubble(int w, int fig_col, int fig_row, int fig_cols) {
    if (!bay_joe.active || !bay_joe.bubble_on) return;
    enum { MAXW = 20, MAXL = 4 };
    char lines[MAXL][MAXW + 1];
    int n = 0, inner = 0;
    const char *p = bay_joe.bubble;
    while (*p && n < MAXL) {                       /* greedy word wrap */
        int len = 0, last_space = -1;
        while (p[len] && len < MAXW) { if (p[len] == ' ') last_space = len; ++len; }
        if (p[len] && last_space > 0) len = last_space;
        memcpy(lines[n], p, (size_t)len);
        lines[n][len] = '\0';
        if (len > inner) inner = len;
        ++n;
        p += len;
        while (*p == ' ') ++p;
    }
    if (n == 0) return;
    int boxw = inner + 4;
    int right = fig_col + fig_cols + 1;
    int col = right, tail_right = 0;
    if (right + boxw - 1 > w) { col = fig_col - boxw - 1; tail_right = 1; }
    if (col < 1) col = 1;
    int row = fig_row < 8 ? 8 : fig_row;
    const char *ink = "\033[1;48;2;255;255;255;38;2;20;20;30m";
    printf("%s\033[%d;%dH╭", ink, row, col);
    for (int i = 0; i < boxw - 2; ++i) printf("─");
    printf("╮");
    for (int i = 0; i < n; ++i) {
        int mid = (i == n / 2);
        printf("\033[%d;%dH%s ", row + 1 + i, col, (mid && !tail_right) ? "◀" : "│");
        printf("%-*s", inner, lines[i]);
        printf(" %s", (mid && tail_right) ? "▶" : "│");
    }
    printf("\033[%d;%dH╰", row + 1 + n, col);
    for (int i = 0; i < boxw - 2; ++i) printf("─");
    printf("╯\033[0m");
}

#endif /* BAYEISHUUI_JOE_H */
