#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <locale.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static struct termios original;
static int raw_enabled = 0, running = 1;
static void restore(void) {
    if (raw_enabled) { tcsetattr(STDIN_FILENO, TCSAFLUSH, &original); raw_enabled = 0; }
    printf("\033[0m\033[?25h\033[2J\033[H");
    fflush(stdout);
}
static void stop_signal(int sig) { (void)sig; running = 0; }
static double clampd(double x, double lo, double hi) { return x < lo ? lo : (x > hi ? hi : x); }
static double now_s(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
static int key_ready(void) {
    fd_set f; struct timeval tv = {0,0}; FD_ZERO(&f); FD_SET(STDIN_FILENO,&f);
    return select(STDIN_FILENO+1,&f,NULL,NULL,&tv) > 0;
}
static void color(int r,int g,int b) { printf("\033[38;2;%d;%d;%dm",r,g,b); }
static void bg(int r,int g,int b) { printf("\033[48;2;%d;%d;%dm",r,g,b); }
static void bar(const char *name, double value, int width, int r, int g, int b) {
    int n=(int)round(clampd(value,0,100)*width/100.0);
    printf("%s ",name); color(220,225,235); printf("[");
    for(int i=0;i<width;i++){ if(i<n) color(r,g,b); else color(65,72,87); printf(i<n?"█":"·"); }
    color(220,225,235); printf("] %3d%%  ",(int)value);
}
static void fighter(int x,int y,int scale,int player,int guard,int strike) {
    const char *ink=player?"\033[38;2;100;210;255m":"\033[38;2;255;105;115m";
    int s=scale<1?1:scale;
    printf("\033[%d;%dH%s",y,x,ink);
    if (s>=3) {
        printf("  O  \n\033[%d;%dH",y+1,x);
        printf(" /|\\ \n\033[%d;%dH",y+2,x);
        if(strike) printf("  |----> \n"); else if(guard) printf(" /|\\  \n"); else printf(" / \\  \n");
        printf("\033[%d;%dH",y+3,x);
        printf(" / \\");
    } else {
        printf(" O \n\033[%d;%dH",y+1,x);
        printf(strike?"-|-==>":" /|\\");
        printf("\033[%d;%dH",y+2,x);
        printf(" / \\");
    }
    printf("\033[0m");
}
static const char *opponents[15] = {"Milo Quickstep","Jax Ironhand","Nia Swift","Bram Stone","Kira Storm","Dante Cross","Sora Blade","Viktor Voss","Mei Dragon","Orion Vale","Zara Night","Cassian Wolf","Rhea Titan","Kael Thunder","The First Fang"};
static const char *guild_ranks[15] = {"Initiate","Copper","Bronze","Silver","Gold","Platinum","Jade","Ruby","Sapphire","Emerald","Obsidian","Master","Grandmaster","Champion","Guild Legend"};
static const char *styles[15] = {
"Muay Thai Fundamentals","Pressure Muay Mat","Long-Guard Muay Femur","Counter-Kicking Muay Tae","Knee-Forward Muay Khao",
"Elbow Specialist Muay Sok","Clinch Controller","Southpaw Angle Fighter","Teep-and-Reset Tactician","High-Guard Pressure Fighter",
"Switch-Kick Specialist","Counter-Elbow Technician","Body-Kick Attrition Fighter","Advanced Muay Femur","Complete Nak Muay"
};
static const char *style_notes[15] = {
"Balanced stance; checks range before committing.","Walks forward behind a tight guard; heavy hands.","Uses long guard and feints to draw attacks.","Punishes entries with timed round kicks.","Closes distance to pressure with knees.",
"Looks for short-range elbow counters.","Seeks clinch range and sustained knee pressure.","Circles off-line to create awkward angles.","Uses teeps to interrupt forward movement.","Blocks first, then answers with combinations.",
"Changes stance and attacks from either side.","Baits attacks, then counters at close range.","Attacks the body and taxes your stamina.","Reads patterns, varies rhythm, and counters.","Mixes every Muay Thai tool with few predictable openings."
};
static void render(int w,int h,double hp,double ehp,double stamina,double estamina,double gap,int px,int ex,int guard,int strike,const char *message,int frame,int rank,int fight_no,int wins,int losses,int ground_mode) {
    int scene=h-9; if(scene<9) scene=9; if(scene>23) scene=23;
    printf("\033[2J\033[H");
    bg(7,13,25); color(130,205,255);
    printf("  BAYEISHUUI  /  FIGHTER GUILD");
    color(255,205,115); printf("  RANK %02d/15: %s\n",rank,guild_ranks[rank-1]);
    color(220,225,235); printf("  OPPONENT: %s | FIGHT %02d/17 | RECORD %d-%d\n  STYLE: %s\n",opponents[rank-1],fight_no,wins,losses,styles[rank-1]);
    color(155,190,220); printf("  TACTIC: %s\n",style_notes[rank-1]);
    bar("YOU ",hp,14,70,220,255); bar("RIVAL",ehp,14,255,90,105); printf("\n");
    bar("STAMINA",stamina,12,95,235,155); bar("RIVAL",estamina,12,255,180,90); printf("\n");
    int horizon=scene/3;
    for(int y=0;y<scene;y++){
        printf("\033[%d;1H",y+8);
        if(y<horizon) { color(30+y*2,42+y*2,62+y*3); for(int x=0;x<w;x++) printf(" "); }
        else {
            for(int x=0;x<w;x++){
                int mid=w/2, dx=abs(x-mid), dy=y-horizon;
                if(x==mid || (dy>0 && dx==dy*3) || (dy>1 && dx==dy*5)) color(55+dy*4,75+dy*3,100+dy*2);
                else if((x+y+frame)%17==0) color(80,105,130);
                else color(26+dy*2,36+dy*2,51+dy*2);
                printf((x==mid || (dy>0 && dx==dy*3) || (dy>1 && dx==dy*5))?"╱":" ");
            }
        }
    }
    int ey=4+horizon+1+(int)((6.0-gap)*0.8); if(ey>scene+1)ey=scene+1;
    int py=4+scene-5; if(py<ey+3)py=ey+3;
    fighter(ex,ey,2,0,0,0); fighter(px,py,3,1,guard,strike);
    printf("\033[%d;1H",h-3); color(255,210,130); printf(" %s",message);
    color(175,190,210); printf("\033[%d;1H W advance  A/D circle  S retreat  SPACE strike  G guard  Q quit",h-1);
    printf("\033[0m"); fflush(stdout);
}
int main(int argc,char **argv) {
    (void)argc; (void)argv; setlocale(LC_ALL,"");
    if(!isatty(STDIN_FILENO)||!isatty(STDOUT_FILENO)){fprintf(stderr,"Run this game in an interactive Linux terminal.\n");return 2;}
    struct winsize ws; ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws);
    int w=ws.ws_col?ws.ws_col:80,h=ws.ws_row?ws.ws_row:30;
    if(w<60||h<24){fprintf(stderr,"Please resize terminal to at least 60x24.\n");return 2;}
    if(tcgetattr(STDIN_FILENO,&original)<0){perror("tcgetattr");return 2;}
    struct termios raw=original; raw.c_lflag &= (tcflag_t) ~(ICANON|ECHO); raw.c_iflag &= (tcflag_t) ~(IXON|ICRNL); raw.c_cc[VMIN]=0; raw.c_cc[VTIME]=0;
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw)<0){perror("tcsetattr");return 2;} raw_enabled=1;
    atexit(restore); signal(SIGINT,stop_signal); signal(SIGTERM,stop_signal);
    printf("\033[?25l");
    double hp=100,ehp=100,st=100,es=100,gap=3.0,last=now_s(),next_enemy=0,player_cd=0,guard_until=0,enemy_wind=0;
    int lateral=0, frame=0, strike=0,guard=0,rank=1; char message[128]="Fighter Guild: defeat all 15 ranked opponents.";
    while(running&&hp>0&&rank<=15){
        double t=now_s(),dt=clampd(t-last,0,.12); last=t;
        strike=0; guard=t<guard_until;
        if(key_ready()){char k=0; if(read(STDIN_FILENO,&k,1)==1){
            if(k=='q'||k=='Q')break;
            if(k=='w'||k=='W'){gap=clampd(gap-.22,.7,6.5);snprintf(message,sizeof message,"You step in behind your guard.");}
            if(k=='s'||k=='S'){gap=clampd(gap+.25,.7,6.5);snprintf(message,sizeof message,"You create space.");}
            if(k=='a'||k=='A'){lateral=clampd(lateral-1,-3,3);snprintf(message,sizeof message,"You circle left.");}
            if(k=='d'||k=='D'){lateral=clampd(lateral+1,-3,3);snprintf(message,sizeof message,"You circle right.");}
            if(k=='g'||k=='G'){guard_until=t+.55;snprintf(message,sizeof message,"Guard up. Time the opponent.");}
            if(ground_mode&&(k=='j'||k=='J')){ground_control++;snprintf(message,sizeof message,"Ground control: %d.",ground_control);}
            else if(ground_mode&&(k=='k'||k=='K')&&t>=player_cd&&st>=10){int dmg=12+rand()%7;ehp=clampd(ehp-dmg,0,100);st-=10;player_cd=t+.7;strike=1;snprintf(message,sizeof message,"GROUND AND POUND! %d damage.",dmg);}
            else if(ground_mode&&(k=='l'||k=='L')&&t>=player_cd&&ground_control>=2){if(rand()%100<35+ground_control*8){ehp=0;ground_wins++;snprintf(message,sizeof message,"SUBMISSION! Bout finished.");}else{ground_control=0;player_cd=t+1.0;snprintf(message,sizeof message,"Submission defended; rebuild control.");}}
            else if(!ground_mode&&(k==' '||k=='e'||k=='E'||k=='r'||k=='R')&&t>=player_cd&&st>=13){
                double reach=(k=='r'||k=='R')?3.15:(k=='e'||k=='E')?2.7:2.25;
                int damage=(k=='r'||k=='R')?5:(k=='e'||k=='E')?11:8;
                const char *technique=(k=='r'||k=='R')?"TEEP":(k=='e'||k=='E')?"ROUND KICK":"STRAIGHT / ELBOW";
                st-=(k=='e'||k=='E')?19:(k=='r'||k=='R')?10:13; player_cd=t+((k=='e'||k=='E')?.78:.62);strike=1;
                if(gap<=reach&&abs(lateral)<=2){int dealt=damage+(rand()%5);ehp=clampd(ehp-dealt,0,100);damage_to_enemy+=dealt;snprintf(message,sizeof message,"%s lands — return to Muay Thai guard.",technique);if(!ground_unlocked&&damage_to_enemy>=150){ground_unlocked=1;ground_mode=1;ground_control=0;snprintf(message,sizeof message,"150%% STRIKING DAMAGE! GROUND GAME UNLOCKED: J control, K ground-and-pound, L submission.");}}
                else snprintf(message,sizeof message,"%s misses. Re-establish range.",technique);
            }
        }}
        st=clampd(st+dt*(guard?3:9),0,100); es=clampd(es+dt*7,0,100);
        if(t>=next_enemy&&gap<3.0){
            double pace=(rank==1?1.0:(rank==2||rank==5||rank==7||rank==10?0.72:(rank==3||rank==4||rank==8||rank==9||rank==11||rank==12||rank==13||rank==14?0.9:0.78)));
            enemy_wind=t+(rank==3||rank==4||rank==8||rank==12||rank==14?0.58:0.40);
            next_enemy=t+(1.65-rank*0.055)*pace+(rand()%100)/100.0;
            if(rank==2||rank==5||rank==6||rank==7) gap=clampd(gap-0.035,.7,6.5);
            if(rank==4||rank==8||rank==11||rank==14) lateral=(lateral==0?1:-lateral);
        }
        if(enemy_wind>0&&t>=enemy_wind){
            if(gap<=(rank==5||rank==6||rank==7?1.65:rank==9?2.7:2.1)&&abs(lateral)<=2){
                int base=(rank==4||rank==11||rank==13?10:rank==5||rank==7?8:6);
                if(guard){hp=clampd(hp-(1+rank/5),0,100);snprintf(message,sizeof message,"CHECK / BLOCK! You absorb the %s.",rank%4==0?"punches":rank%4==1?"round kick":rank%4==2?"teep":"knee / elbow");}
                else {hp=clampd(hp-(base+rank/2+(rand()%7)),0,100);snprintf(message,sizeof message,"%s lands %s — defend and counter.",opponents[rank-1],rank%4==0?"straight punches":rank%4==1?"round kick":rank%4==2?"teep":"knee / elbow");}
            }
            else snprintf(message,sizeof message,"The opponent's strike misses at this range.");
            enemy_wind=0;
        }
        if(gap>2.0&&frame%4==0)gap=clampd(gap-.025,.7,6.5);
        render(w,h,hp,ehp,st,es,gap,w/2-3+lateral*2,w/2-2,guard,strike,message,frame++,rank,fight_no,wins,losses,ground_mode);
        struct timespec pause={0,50000000}; nanosleep(&pause,NULL);
        if(ehp<=0){wins++;total_wins++;fight_no++;ground_mode=ground_unlocked;ground_control=0;ehp=100;es=100;st=100;gap=3.2;lateral=0;enemy_wind=0;next_enemy=t+1.0;player_cd=t+.5;hp=100;
            if(fight_no>17){int rating=(int)round(wins/17.0*100.0);const char *kind=ground_wins>wins/2?"FINISHER":(ground_wins==0?"STRIKER":"BALANCED STRIKER / FINISHER");snprintf(message,sizeof message,"%s rates you %d/100 — %s.",opponents[rank-1],rating,kind);if(rank==15){rank=16;break;}rank++;fight_no=1;wins=0;damage_to_enemy=0;ground_unlocked=0;ground_wins=0;}else snprintf(message,sizeof message,"BOUT WON — fight %d/17, wins %d, losses %d.",fight_no,wins,losses);
        }
        if(hp<=0){losses++;total_losses++;if(!sudden_death_used&&total_wins>=11&&total_wins>total_losses){sudden_death_used=1;hp=25;ehp=100;snprintf(message,sizeof message,"SUDDEN DEATH! Eligible: 11+ wins and ahead.");}else{running=0;snprintf(message,sizeof message,"KNOCKOUT — Sudden Death requires 11+ wins and a winning record.");}}
    }
    printf("\033[0m\033[?25h\n%s\n",hp<=0?"Campaign over — you were defeated.":rank>15?"FIGHTER GUILD COMPLETE — all 15 opponents defeated!":"Campaign abandoned.");
    return 0;
}
