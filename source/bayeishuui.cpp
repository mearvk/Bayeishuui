#include <algorithm>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include "bayeishuui_sprites.h"
#include "bayeishuui_ai.h"

namespace {
termios original{};
bool rawEnabled=false, running=true;
void restore() {
    if(rawEnabled) { tcsetattr(STDIN_FILENO,TCSAFLUSH,&original); rawEnabled=false; }
    std::cout << "\033[0m\033[?25h\033[2J\033[H" << std::flush;
}
void stop(int) { running=false; }
double clamp(double x,double lo,double hi) { return std::max(lo,std::min(x,hi)); }
using Clock=std::chrono::steady_clock;
double seconds() { return std::chrono::duration<double>(Clock::now().time_since_epoch()).count(); }
bool keyReady() { fd_set f; timeval tv{0,0}; FD_ZERO(&f); FD_SET(STDIN_FILENO,&f); return select(STDIN_FILENO+1,&f,nullptr,nullptr,&tv)>0; }
void fg(int r,int g,int b) { std::cout << "\033[38;2;"<<r<<';'<<g<<';'<<b<<'m'; }
void bar(const char* label,double value,int width,int r,int g,int b) {
    int filled=static_cast<int>(std::round(clamp(value,0,100)*width/100));
    std::cout<<label<<" "; fg(220,225,235); std::cout<<'[';
    for(int i=0;i<width;++i) { if(i<filled)fg(r,g,b);else fg(65,72,87);std::cout<<(i<filled?"█":"·"); }
    fg(220,225,235); std::cout<<"] "<<static_cast<int>(value)<<"%  ";
}
const char* opponents[15]={"Milo Quickstep","Jax Ironhand","Nia Swift","Bram Stone","Kira Storm","Dante Cross","Sora Blade","Viktor Voss","Mei Dragon","Orion Vale","Zara Night","Cassian Wolf","Rhea Titan","Kael Thunder","The First Fang"};
const char* guildRanks[15]={"Initiate","Copper","Bronze","Silver","Gold","Platinum","Jade","Ruby","Sapphire","Emerald","Obsidian","Master","Grandmaster","Champion","Guild Legend"};
const char* styles[15]={
"Muay Thai Fundamentals","Pressure Muay Mat","Long-Guard Muay Femur","Counter-Kicking Muay Tae","Knee-Forward Muay Khao",
"Elbow Specialist Muay Sok","Clinch Controller","Southpaw Angle Fighter","Teep-and-Reset Tactician","High-Guard Pressure Fighter",
"Switch-Kick Specialist","Counter-Elbow Technician","Body-Kick Attrition Fighter","Advanced Muay Femur","Complete Nak Muay"};
const char* styleNotes[15]={
"Balanced stance; checks range before committing.","Walks forward behind a tight guard; heavy hands.","Uses long guard and feints to draw attacks.","Punishes entries with timed round kicks.","Closes distance to pressure with knees.",
"Looks for short-range elbow counters.","Seeks clinch range and sustained knee pressure.","Circles off-line to create awkward angles.","Uses teeps to interrupt forward movement.","Blocks first, then answers with combinations.",
"Changes stance and attacks from either side.","Baits attacks, then counters at close range.","Attacks the body and taxes your stamina.","Reads patterns, varies rhythm, and counters.","Mixes every Muay Thai tool with few predictable openings."};
void render(int w,int h,double hp,double enemyHp,double stamina,double enemyStamina,double gap,int px,int ex,bool guarding,bool striking,const std::string& message,int frame,int rank,int fightNo,int wins,int losses,bool groundMode,bool aiMode) {
    int scene=std::clamp(h-9,9,23), horizon=scene/3;
    std::cout<<"\033[2J\033[H"; fg(130,205,255);
    std::cout<<"  BAYEISHUUI  /  FIGHTER GUILD";fg(255,205,115);
    std::cout<<"  RANK "<<(rank<10?"0":"")<<rank<<"/15: "<<guildRanks[rank-1]<<"\n";
    fg(220,225,235);std::cout<<"  OPPONENT: "<<opponents[rank-1]<<" | DISTANCE "<<static_cast<int>(gap*10)/10.0<<"m | FIGHT "<<fightNo<<"/17 | RECORD "<<wins<<"-"<<losses<<"\n";
    fg(255,155,120);std::cout<<"  STYLE: "<<styles[rank-1]<<"\n";
    fg(155,190,220);std::cout<<"  TACTIC: "<<styleNotes[rank-1]<<"\n";
    bar("YOU HP",hp,14,70,220,255);bar("RIVAL HP",enemyHp,14,255,90,105);std::cout<<'\n';if(groundMode){fg(255,190,90);std::cout<<"  GROUND GAME: J control | K ground-and-pound | L submission\n";}else{fg(155,190,220);std::cout<<"  STRIKING: Space punch | R teep | E round kick | G guard\n";}
    bar("STAMINA",stamina,12,95,235,155);bar("RIVAL",enemyStamina,12,255,180,90);std::cout<<'\n';
    for(int y=0;y<scene;++y) {
        std::cout<<"\033["<<y+8<<";1H";
        if(y<horizon) { fg(30+y*2,42+y*2,62+y*3);std::cout<<std::string(w,' '); }
        else for(int x=0;x<w;++x) {
            int dx=std::abs(x-w/2),dy=y-horizon;
            bool line=(x==w/2)||(dy>0&&dx==dy*3)||(dy>1&&dx==dy*5);
            if(line)fg(55+dy*4,75+dy*3,100+dy*2);else if((x+y+frame)%17==0)fg(80,105,130);else fg(26+dy*2,36+dy*2,51+dy*2);
            std::cout<<(line?"╱":" ");
        }
    }
    int ey=std::min(scene+1,4+horizon+1+static_cast<int>((6.0-gap)*.8));
    int py=std::max(4+scene-5,ey+3);
    bay_sprite_draw(ex,ey,0,rank-1,frame,false,false,groundMode);bay_sprite_draw(px,py,1,0,frame,guarding,striking,groundMode);
    fg(255,210,130);std::cout<<"\033["<<h-3<<";1H "<<(aiMode?"PERFECT FIGHTER: ":"")<<message;
    fg(175,190,210);std::cout<<"\033["<<h-1<<";1H W advance  A/D circle  S retreat  SPACE/R/E strike  G guard  J/K/L ground  Q quit";
    std::cout<<"\033[0m"<<std::flush;
}
}
int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)^static_cast<std::time_t>(getpid())));
    if(!isatty(STDIN_FILENO)||!isatty(STDOUT_FILENO)){std::cerr<<"Run this game in an interactive Linux terminal.\n";return 2;}
    winsize ws{};ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws);int w=ws.ws_col?ws.ws_col:80,h=ws.ws_row?ws.ws_row:30;
    if(w<60||h<24){std::cerr<<"Please resize terminal to at least 60x24.\n";return 2;}
    if(tcgetattr(STDIN_FILENO,&original)<0){perror("tcgetattr");return 2;}
    termios raw=original;raw.c_lflag&=static_cast<tcflag_t>(~(ICANON|ECHO));raw.c_iflag&=static_cast<tcflag_t>(~(IXON|ICRNL));raw.c_cc[VMIN]=0;raw.c_cc[VTIME]=0;
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw)<0){perror("tcsetattr");return 2;}rawEnabled=true;
    std::atexit(restore);std::signal(SIGINT,stop);std::signal(SIGTERM,stop);std::cout<<"\033[?25l";
    double hp=100,enemyHp=100,stamina=100,enemyStamina=100,gap=3,last=seconds(),nextEnemy=0,playerCooldown=0,guardUntil=0,enemyWind=0;
    int lateral=0,frame=0,rank=1,fightNo=1,wins=0,losses=0,groundControl=0,groundWins=0;bool strike=false,guard=false,groundMode=false,groundUnlocked=false,suddenDeathUsed=false;int totalWins=0,totalLosses=0;double damageToEnemy=0,sessionStart=0,aiNextAction=0;bool aiMode=true;
    std::string message="Double SPACE within 21 seconds for manual Player 1; otherwise Perfect Fighter.";
    std::cout<<"\033[2J\033[H\033[?25lBAYEISHUUI — PLAYER 1 SELECT\n\nDouble-tap SPACE within 21 seconds for manual play.\nOtherwise Perfect Fighter plays Player 1.\nPress ESC to cancel.\n"<<std::flush;
    double choiceStart=seconds(),lastSpace=-10.0;
    while(running&&seconds()-choiceStart<21.0){if(keyReady()){char ck=0;if(::read(STDIN_FILENO,&ck,1)==1){double kt=seconds();if(ck==27){running=false;break;}if(ck==' '&&kt-lastSpace<=0.65){aiMode=false;break;}if(ck==' ')lastSpace=kt;}}
      std::cout<<"\033[7;1HStarting Perfect Fighter in "<<(21-static_cast<int>(seconds()-choiceStart))<<" seconds...  "<<std::flush;std::this_thread::sleep_for(std::chrono::milliseconds(50));}
    if(!running){std::cout<<"\nSession cancelled.\n";return 0;}sessionStart=seconds();last=sessionStart;
    message=aiMode?"PERFECT FIGHTER ACTIVE — live actions; ESC ends session.":"MANUAL PLAYER 1 — your controls are active.";
    while(running&&hp>0&&rank<=15) {
        double t=seconds(),dt=clamp(t-last,0,.12);last=t;strike=false;guard=t<guardUntil;
        if(keyReady()||aiMode){char k=0;bool gotKey=false;if(aiMode){if(keyReady()){char uk=0;if(::read(STDIN_FILENO,&uk,1)==1&&uk==27){running=false;message="ESCAPE — Perfect Fighter session ended.";}}
 if(running&&t>=aiNextAction){k=bayeishuui_ai_action(hp,enemyHp,stamina,gap,lateral,groundMode,groundUnlocked,groundControl,enemyWind,t,playerCooldown);gotKey=(k!='\0');aiNextAction=t+.22;}}
 else if(::read(STDIN_FILENO,&k,1)==1)gotKey=true;
 if(gotKey){
            if(k=='q'||k=='Q')break;
            if(k=='w'||k=='W'){gap=clamp(gap-.22,.7,6.5);message="You step in behind your guard.";}
            if(k=='s'||k=='S'){gap=clamp(gap+.25,.7,6.5);message="You create space.";}
            if(k=='a'||k=='A'){lateral=static_cast<int>(clamp(lateral-1,-3,3));message="You circle left.";}
            if(k=='d'||k=='D'){lateral=static_cast<int>(clamp(lateral+1,-3,3));message="You circle right.";}
            if(k=='g'||k=='G'){guardUntil=t+.55;message="Guard up. Time the opponent.";}
            if(groundMode&&(k=='j'||k=='J')){++groundControl;message="Ground control: "+std::to_string(groundControl)+".";}
            else if(groundMode&&(k=='k'||k=='K')&&t>=playerCooldown&&stamina>=10){int dmg=12+std::rand()%7;enemyHp=clamp(enemyHp-dmg,0,100);if(enemyHp<=0)++groundWins;stamina-=10;playerCooldown=t+.7;strike=true;message="GROUND AND POUND! "+std::to_string(dmg)+" damage.";}
            else if(groundMode&&(k=='l'||k=='L')&&t>=playerCooldown&&groundControl>=2){if(std::rand()%100<35+groundControl*8){enemyHp=0;++groundWins;message="SUBMISSION! Bout finished.";}else{groundControl=0;playerCooldown=t+1.0;message="Submission defended; rebuild control.";}}
            else if(!groundMode&&(k==' '||k=='e'||k=='E'||k=='r'||k=='R')&&t>=playerCooldown&&stamina>=13){
                double reach=(k=='r'||k=='R')?3.15:(k=='e'||k=='E')?2.7:2.25;
                int damage=(k=='r'||k=='R')?5:(k=='e'||k=='E')?11:8;
                const char* technique=(k=='r'||k=='R')?"TEEP":(k=='e'||k=='E')?"ROUND KICK":"STRAIGHT / ELBOW";
                stamina-=(k=='e'||k=='E')?19:(k=='r'||k=='R')?10:13;playerCooldown=t+((k=='e'||k=='E')?.78:.62);strike=true;
                if(gap<=reach&&std::abs(lateral)<=2){int dealt=damage+std::rand()%5;enemyHp=clamp(enemyHp-dealt,0,100);damageToEnemy+=dealt;message=std::string(technique)+" lands — return to Muay Thai guard.";if(!groundUnlocked&&damageToEnemy>=150){groundUnlocked=true;groundMode=true;groundControl=0;message="150% STRIKING DAMAGE! GROUND GAME UNLOCKED: J control, K ground-and-pound, L submission.";}}
                else message=std::string(technique)+" misses. Re-establish range.";
            }
        }}
        stamina=clamp(stamina+dt*(guard?3:9),0,100);enemyStamina=clamp(enemyStamina+dt*7,0,100);
        if(t>=nextEnemy&&gap<3.0){
            double pace=(rank==1?1.0:(rank==2||rank==5||rank==7||rank==10?.72:(rank==3||rank==4||rank==8||rank==9||rank==11||rank==12||rank==13||rank==14?.9:.78)));
            enemyWind=t+(rank==3||rank==4||rank==8||rank==12||rank==14?.58:.40);
            nextEnemy=t+(1.65-rank*.055)*pace+(std::rand()%100)/100.0;
            if(rank==2||rank==5||rank==6||rank==7)gap=clamp(gap-.035,.7,6.5);
            if(rank==4||rank==8||rank==11||rank==14)lateral=(lateral==0?1:-lateral);
        }
        if(enemyWind>0&&t>=enemyWind){
            if(gap<=(rank==5||rank==6||rank==7?1.65:rank==9?2.7:2.1)&&std::abs(lateral)<=2){
                int base=(rank==4||rank==11||rank==13?10:rank==5||rank==7?8:6);
                if(guard){hp=clamp(hp-(1+rank/5),0,100);message=std::string("CHECK / BLOCK! You absorb the ")+(rank%4==0?"straight punches":rank%4==1?"round kick":rank%4==2?"teep":"knee / elbow")+".";}
                else {hp=clamp(hp-(base+rank/2+std::rand()%7),0,100);message=std::string(opponents[rank-1])+" lands "+(rank%4==0?"straight punches":rank%4==1?"round kick":rank%4==2?"teep":"knee / elbow")+" — defend and counter.";}
            }
            else message="The opponent's strike misses at this range.";
            enemyWind=0;
        }
        if(gap>2.0&&frame%4==0)gap=clamp(gap-.025,.7,6.5);
        render(w,h,hp,enemyHp,stamina,enemyStamina,gap,w/2-3+lateral*2,w/2-2,guard,strike,message,frame++,rank,fightNo,wins,losses,groundMode,aiMode);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if(enemyHp<=0){++wins;++totalWins;++fightNo;groundMode=groundUnlocked;groundControl=0;enemyHp=100;enemyStamina=100;stamina=100;gap=3.2;lateral=0;enemyWind=0;nextEnemy=t+1.0;playerCooldown=t+.5;hp=100;
            if(fightNo>17){int rating=80+groundWins*5;if(rating>100)rating=100;const char* kind=groundWins>=3?"FINISHER":(groundWins==0?"STRIKER":"BALANCED STRIKER / FINISHER");message=std::string(opponents[rank-1])+" rates you "+std::to_string(rating)+"/100 — "+kind+".";if(rank==15){rank=16;break;}++rank;fightNo=1;wins=0;damageToEnemy=0;groundUnlocked=false;groundWins=0;}else message="BOUT WON — fight "+std::to_string(fightNo)+"/17, wins "+std::to_string(wins)+", losses "+std::to_string(losses)+".";
        }
        if(hp<=0){++losses;++totalLosses;if(!suddenDeathUsed&&totalWins>=11&&totalWins>totalLosses){suddenDeathUsed=true;hp=25;enemyHp=100;message="SUDDEN DEATH! Eligible: 11+ wins and ahead.";}else{running=false;message="KNOCKOUT — Sudden Death requires 11+ wins and a winning record.";}}
    }
    std::cout<<"\033[0m\033[?25h\n"<<(hp<=0?"Campaign over — you were defeated.":rank>15?"FIGHTER GUILD COMPLETE — all 15 opponents, 17 fights each!":"Campaign abandoned.")<<'\n'<<message<<'\n';
    bayeishuui_save_run_config(seconds()-sessionStart,rank>15,aiMode?1:0,totalWins,totalLosses);
    return 0;
}
