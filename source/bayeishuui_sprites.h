#ifndef BAYEISHUUI_SPRITES_H
#define BAYEISHUUI_SPRITES_H

/*
 * BAYEISHUUI TERMINAL SPRITE SYSTEM
 * Branded ANSI/Unicode sprite animation for the Fighter Guild.
 *
 * Every combatant uses a consecutive 244-frame animation cycle (indices
 * 0..243). The frames are composed procedurally from authored terminal poses,
 * transition beats, style timing, and action overrides. This keeps the sprites
 * crisp in a terminal without shipping thousands of redundant text frames.
 */
#include <stdio.h>

#define BAY_SPRITE_FRAME_COUNT 244
#define BAY_SPRITE_POSE_COUNT 16

static const char *const bay_sprite_poses[BAY_SPRITE_POSE_COUNT][5] = {
    {"   O     ", "  /|\\    ", "   |     ", "  / \\    ", "  / \\    "}, /* ready */
    {"   O     ", "  \\|/    ", "   |     ", "  / \\    ", "  / \\    "}, /* guard rise */
    {"   O     ", " _/|      ", "   |\\    ", "  /  \\   ", "  /  \\   "}, /* step in */
    {"   O     ", "  /|\\    ", "   |  \\  ", "  /   >   ", "  /       "}, /* teep chamber */
    {"   O----- ", "  /|      ", "   |      ", "  / \\    ", "  / \\    "}, /* straight */
    {"   O     ", "  /|\\----", "   |      ", "  / \\    ", "  / \\    "}, /* long guard */
    {"   O     ", "  /|\\    ", "   |\\    ", "  /  \\   ", " /    >   "}, /* round kick */
    {"   O     ", "  /|\\    ", "   |\\    ", "  /  >    ", "  /       "}, /* knee */
    {"   O--    ", "  /|      ", "   |      ", "  / \\    ", "  / \\    "}, /* elbow */
    {"  _O     ", "   |\\    ", "  /|      ", "  / \\    ", "  / \\    "}, /* slip counter */
    {"   O     ", "  /|\\    ", "   |      ", " _/ \\    ", "  /  \\   "}, /* pivot */
    {"   O     ", "  /|\\    ", "  _|      ", " /   \\   ", " /    \\  "}, /* impact */
    {"   O     ", "  /|\\    ", "   |      ", " _/ \\    ", " /   \\   "}, /* recoil */
    {"   O____ ", " __|      ", "   |      ", "  / \\    ", " /   \\   "}, /* ground control */
    {"  _O____ ", " __|      ", "   |      ", "  / \\    ", " /   \\   "}, /* ground and pound */
    {"  _O_    ", " /___\\   ", "    \\   ", "  /  \\   ", " /    \\  "}  /* finish / submission */
};

static const char *const bay_sprite_style_arms[15] = {
    "  /|\\    ", "  \\|/    ", " _/|\\    ", "  /|---->", "  /|\\    ",
    "  /|--    ", "  \\|/    ", " _/|\\    ", "  /|---->", "  \\|/    ",
    "  /|\\    ", "  /|--    ", "  /|\\    ", "  \\|/    ", "  /|\\    "
};
static const char *const bay_sprite_style_legs[15] = {
    "  / \\    ", " /  \\    ", "  / \\    ", " /   >    ", "  /  >    ",
    "  / \\    ", " /  \\    ", " _/ \\    ", " /   >    ", "  / \\    ",
    " /  \\    ", "  / \\    ", " /   >    ", "  / \\    ", " /  \\    "
};

static inline void bay_sprite_color(int player, int style) {
    static const unsigned char palette[15][3] = {
        {255,170,105},{245,130,95},{125,220,255},{255,190,100},{190,150,255},
        {255,115,135},{105,235,205},{200,220,255},{255,210,120},{130,190,255},
        {235,130,210},{255,155,115},{155,235,145},{235,220,155},{255,245,220}
    };
    if (player) printf("\033[38;2;90;220;255m");
    else {
        int i = style < 0 ? 0 : style % 15;
        printf("\033[38;2;%u;%u;%um", palette[i][0], palette[i][1], palette[i][2]);
    }
}

/* Draw one of the 244 connected terminal animation frames. */
static inline void bay_sprite_draw(int x, int y, int player, int style,
                                   int frame, int guarding, int striking,
                                   int ground_mode) {
    int f = frame % BAY_SPRITE_FRAME_COUNT;
    if (f < 0) f += BAY_SPRITE_FRAME_COUNT;

    /*
     * Sixteen authored pose families cover stance, footwork, punches, teep,
     * round kick, knee, elbow, slip/counter, impact, recoil, ground control,
     * ground-and-pound, and finish. 244 frames distribute across them without
     * gaps; a style offset changes each fighter's rhythm while preserving the
     * consecutive 0..243 frame sequence.
     */
    int styled = (f + ((style < 0 ? 0 : style) * 7)) % BAY_SPRITE_FRAME_COUNT;
    int pose = styled / 15;
    int beat = styled % 15;
    if (pose >= BAY_SPRITE_POSE_COUNT) pose = BAY_SPRITE_POSE_COUNT - 1;
    int next_pose = (pose + 1) % BAY_SPRITE_POSE_COUNT;

    if (ground_mode) pose = beat < 5 ? 13 : (beat < 10 ? 14 : 15);
    else if (striking) pose = (style % 3 == 0) ? 4 : ((style % 3 == 1) ? 6 : 8);
    else if (guarding) pose = beat < 8 ? 1 : 5;
    else if (pose == 13 || pose == 14 || pose == 15) pose = beat < 8 ? 0 : 10;
    next_pose = (pose + 1) % BAY_SPRITE_POSE_COUNT;

    bay_sprite_color(player, style);
    for (int row = 0; row < 5; ++row) {
        const char *line = bay_sprite_poses[pose][row];
        /*
         * Each 15-frame beat has three connected stages: held key pose,
         * alternating limb transition, and arrival at the next key pose.
         * The renderer advances through all 244 consecutive frame indices.
         */
        if (!ground_mode && !guarding && !striking && beat >= 5 && beat < 10 &&
            ((beat + row + (style < 0 ? 0 : style)) % 2 == 1))
            line = bay_sprite_poses[next_pose][row];
        else if (!ground_mode && !guarding && !striking && beat >= 10)
            line = bay_sprite_poses[next_pose][row];
        if (!ground_mode && !guarding && !striking && (pose == 0 || pose == 1 || pose == 10)) {
            int style_id = (style < 0 ? 0 : style % 15);
            if (row == 1) line = bay_sprite_style_arms[style_id];
            if (row == 3) line = bay_sprite_style_legs[style_id];
        }
        if (ground_mode && beat >= 5 && row == 1)
            line = (beat % 2) ? " __|____ " : "  _|____ ";
        printf("\033[%d;%dH", y + row, x);
        printf("%s", line);
    }
    printf("\033[0m");
}

#endif
