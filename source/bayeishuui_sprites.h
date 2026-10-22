#ifndef BAYEISHUUI_SPRITES_H
#define BAYEISHUUI_SPRITES_H

/*
 * BAYEISHUUI — proprietary-feeling, terminal-native fighter sprite system.
 * Every fighter cycles through 244 consecutive frames (0..243). Each frame
 * interpolates a skeletal pose into a fixed-width Unicode/ASCII terminal
 * canvas, so the motion is connected without shipping raster files.
 */
#include <stdio.h>
#include <stdlib.h>

#define BAY_SPRITE_FRAME_COUNT 244
#define BAY_SPRITE_POSE_COUNT 16
#define BAY_SPRITE_W 11
#define BAY_SPRITE_H 7

typedef struct BaySpritePose {
    int hx,hy, nx,ny, sx,sy;
    int lex,ley, lwx,lwy, rex,rey, rwx,rwy;
    int hipx,hipy, lkx,lky, lfx,lfy, rkx,rky, rfx,rfy;
} BaySpritePose;

/* Head, neck, shoulders, elbows/wrists, hip, knees and feet for each key pose. */
static const BaySpritePose bay_sprite_poses[BAY_SPRITE_POSE_COUNT] = {
    {5,0,5,1,5,2, 3,2,2,2, 7,2,8,2, 5,4,4,5,3,6,6,5,7,6}, /* ready */
    {5,0,5,1,5,2, 3,1,3,0, 7,1,7,0, 5,4,4,5,3,6,6,5,7,6}, /* high guard */
    {5,0,5,1,5,2, 3,2,2,1, 7,2,8,1, 5,4,4,5,3,6,7,5,8,6}, /* step in */
    {5,0,5,1,5,2, 3,2,2,1, 7,2,8,2, 5,4,6,4,8,3,4,5,3,6}, /* teep chamber */
    {5,0,5,1,5,2, 3,2,2,1, 7,2,10,2, 5,4,4,5,3,6,6,5,7,6}, /* straight punch */
    {5,0,5,1,5,2, 3,1,2,1, 7,1,9,1, 5,4,4,5,3,6,6,5,7,6}, /* long guard */
    {5,0,5,1,5,2, 3,2,2,1, 7,2,8,1, 5,4,7,3,10,2,4,5,3,6}, /* round kick */
    {5,0,5,1,5,2, 3,2,2,1, 7,2,8,2, 5,4,7,3,8,2,4,5,3,6}, /* knee */
    {5,0,5,1,5,2, 3,2,2,1, 7,2,9,1, 5,4,4,5,3,6,6,5,7,6}, /* elbow */
    {4,0,4,1,5,2, 3,1,2,1, 7,2,8,2, 5,4,4,5,3,6,6,5,7,6}, /* slip/counter */
    {6,0,6,1,5,2, 3,2,2,1, 7,1,8,0, 5,4,3,5,2,6,7,5,8,6}, /* pivot */
    {5,0,5,1,5,2, 3,2,2,2, 7,2,8,1, 5,4,4,5,3,6,7,5,9,5}, /* impact */
    {4,0,5,1,5,2, 3,2,2,3, 7,2,8,2, 5,4,3,5,2,6,7,5,8,6}, /* recoil */
    {2,4,3,4,4,4, 4,5,5,5, 5,3,6,3, 7,4,8,4,9,4,8,5,9,5}, /* ground control */
    {2,4,3,4,4,4, 4,5,6,5, 5,3,7,2, 7,4,8,4,9,4,8,5,9,5}, /* ground-and-pound */
    {2,4,3,4,4,4, 4,5,5,5, 5,3,6,3, 7,4,8,4,10,4,8,5,9,5} /* finish */
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

static inline void bay_sprite_plot(char canvas[BAY_SPRITE_H][BAY_SPRITE_W + 1], int x, int y, char c) {
    if (x >= 0 && x < BAY_SPRITE_W && y >= 0 && y < BAY_SPRITE_H)
        canvas[y][x] = c;
}

static inline void bay_sprite_line(char canvas[BAY_SPRITE_H][BAY_SPRITE_W + 1],
                                   int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        char c = (x0 == x1) ? '|' : (y0 == y1) ? '-' : ((x1-x0)*(y1-y0) < 0 ? '/' : '\\');
        bay_sprite_plot(canvas, x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static inline int bay_sprite_lerp(int a, int b, int numerator, int denominator) {
    return a + (b - a) * numerator / denominator;
}

/* Render a unique interpolated frame for each index in the 244-frame cycle. */
static inline void bay_sprite_draw(int x, int y, int player, int style,
                                   int frame, int guarding, int striking,
                                   int ground_mode) {
    int f = frame % BAY_SPRITE_FRAME_COUNT;
    if (f < 0) f += BAY_SPRITE_FRAME_COUNT;
    int style_id = style < 0 ? 0 : style % 15;
    int styled = (f + style_id * 7) % BAY_SPRITE_FRAME_COUNT;
    int scaled = styled * BAY_SPRITE_POSE_COUNT;
    int pose_id = scaled / BAY_SPRITE_FRAME_COUNT;
    int fraction = scaled % BAY_SPRITE_FRAME_COUNT;
    int next_id = (pose_id + 1) % BAY_SPRITE_POSE_COUNT;
    int denominator = BAY_SPRITE_FRAME_COUNT;
    BaySpritePose p = bay_sprite_poses[pose_id];
    BaySpritePose q = bay_sprite_poses[next_id];

    if (ground_mode) {
        pose_id = 13 + ((styled * 3) / BAY_SPRITE_FRAME_COUNT);
        if (pose_id > 15) pose_id = 15;
        next_id = pose_id == 15 ? 13 : pose_id + 1;
        p = bay_sprite_poses[pose_id]; q = bay_sprite_poses[next_id];
        fraction = (styled * 3) % BAY_SPRITE_FRAME_COUNT;
    } else if (striking) {
        pose_id = style_id % 3 == 0 ? 4 : style_id % 3 == 1 ? 6 : 8;
        p = bay_sprite_poses[pose_id]; q = bay_sprite_poses[pose_id == 4 ? 10 : 0];
        fraction = styled % 61;
        denominator = 61;
    } else if (guarding) {
        pose_id = styled % 2 ? 1 : 5;
        p = bay_sprite_poses[pose_id]; q = bay_sprite_poses[pose_id];
        fraction = 0;
    }

    int v[24], a[24], b[24];
    const int *unused = NULL;
    (void)unused;
    a[0]=p.hx;a[1]=p.hy;a[2]=p.nx;a[3]=p.ny;a[4]=p.sx;a[5]=p.sy;
    a[6]=p.lex;a[7]=p.ley;a[8]=p.lwx;a[9]=p.lwy;a[10]=p.rex;a[11]=p.rey;a[12]=p.rwx;a[13]=p.rwy;
    a[14]=p.hipx;a[15]=p.hipy;a[16]=p.lkx;a[17]=p.lky;a[18]=p.lfx;a[19]=p.lfy;a[20]=p.rkx;a[21]=p.rky;a[22]=p.rfx;a[23]=p.rfy;
    b[0]=q.hx;b[1]=q.hy;b[2]=q.nx;b[3]=q.ny;b[4]=q.sx;b[5]=q.sy;
    b[6]=q.lex;b[7]=q.ley;b[8]=q.lwx;b[9]=q.lwy;b[10]=q.rex;b[11]=q.rey;b[12]=q.rwx;b[13]=q.rwy;
    b[14]=q.hipx;b[15]=q.hipy;b[16]=q.lkx;b[17]=q.lky;b[18]=q.lfx;b[19]=q.lfy;b[20]=q.rkx;b[21]=q.rky;b[22]=q.rfx;b[23]=q.rfy;
    for (int i=0;i<24;++i) v[i]=bay_sprite_lerp(a[i],b[i],fraction,denominator);

    /* Signature stance: 15 style identities vary lead side and foot placement. */
    int side = style_id % 3;
    if (!ground_mode && !striking && !guarding) {
        if (side == 1) { v[6]--; v[8]--; v[20]++; v[22]++; }
        else if (side == 2) { v[10]++; v[12]++; v[16]--; v[18]--; }
    }

    char canvas[BAY_SPRITE_H][BAY_SPRITE_W + 1];
    for (int row=0;row<BAY_SPRITE_H;++row) {
        for (int col=0;col<BAY_SPRITE_W;++col) canvas[row][col]=' ';
        canvas[row][BAY_SPRITE_W]='\\0';
    }
    /* Head, spine, shoulders, arms, hips, knees and feet form a readable glyph fighter. */
    bay_sprite_plot(canvas,v[0],v[1],'O');
    bay_sprite_line(canvas,v[2],v[3],v[4],v[5]);
    bay_sprite_line(canvas,v[2],v[3],v[14],v[15]);
    bay_sprite_line(canvas,v[4],v[5],v[6],v[7]); bay_sprite_line(canvas,v[6],v[7],v[8],v[9]);
    bay_sprite_line(canvas,v[4],v[5],v[10],v[11]); bay_sprite_line(canvas,v[10],v[11],v[12],v[13]);
    bay_sprite_line(canvas,v[14],v[15],v[16],v[17]); bay_sprite_line(canvas,v[16],v[17],v[18],v[19]);
    bay_sprite_line(canvas,v[14],v[15],v[20],v[21]); bay_sprite_line(canvas,v[20],v[21],v[22],v[23]);
    bay_sprite_color(player, style);
    for (int row=0;row<BAY_SPRITE_H;++row) {
        printf("\033[%d;%dH",y+row,x);
        printf("%s",canvas[row]);
    }
    printf("\033[0m");
}

#endif
