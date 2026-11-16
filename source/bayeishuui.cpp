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
#include "bayeishuui_pixels.h"
#include "bayeishuui_ai.h"
#include "bayeishuui_law.h"
#include "bayeishuui_hair.h"
#include "bayeishuui_barber.h"

namespace {
termios original{};
bool rawEnabled=false, running=true;
/* Rendering backend chosen once at startup: kitty/sixel pixels where the
 * terminal supports them, otherwise the universal glyph sprite. */
BayBackend g_backend=BAY_BACKEND_GLYPH;
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
    fg(220,225,235);std::cout<<"  OPPONENT: "<<bay_joe_name_or(opponents[rank-1])<<(bay_joe.active?" (" BAY_JOE_RANK_TITLE ")":"")<<" | DISTANCE "<<static_cast<int>(gap*10)/10.0<<"m | FIGHT "<<fightNo<<"/17 | RECORD "<<wins<<"-"<<losses<<"\n";
    fg(255,155,120);std::cout<<"  STYLE: "<<(bay_joe.active?BAY_JOE_STYLE:styles[rank-1])<<"\n";
    fg(155,190,220);std::cout<<"  TACTIC: "<<(bay_joe.active?BAY_JOE_TACTIC:styleNotes[rank-1])<<"\n";
    bar("YOU HP",hp,14,70,220,255);bar("RIVAL HP",enemyHp,14,255,90,105);if(bay_joe.active){fg(255,205,115);std::cout<<"JOE "<<bay_joe_hp_now(enemyHp)<<"/"<<BAY_JOE_MAX_HP<<" HP";}std::cout<<'\n';if(groundMode){fg(255,190,90);std::cout<<"  GROUND GAME: J control | K ground-and-pound | L submission\n";}else{fg(155,190,220);std::cout<<"  STRIKING: Space punch | R teep | E round kick | G guard\n";}
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
    bay_pixel_draw_backend(g_backend,ex,ey,0,bay_joe.active?0:rank-1,frame,false,false,groundMode);bay_pixel_draw_backend(g_backend,px,py,1,0,frame,guarding,striking,groundMode);
    std::cout<<std::flush;bay_joe_draw_bubble(w,ex,ey,(g_backend==BAY_BACKEND_GLYPH||g_backend==BAY_BACKEND_UNKNOWN)?BAY_SPRITE_W:BAY_PX_W/(BAY_PX_CELL_W/2));fflush(stdout);
    fg(255,210,130);std::cout<<"\033["<<h-3<<";1H "<<(aiMode?"PERFECT FIGHTER: ":"")<<message<<std::flush;
    bay_law_draw_status(h-2);
    bay_hair_draw_hud(w);
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
    std::atexit(restore);std::signal(SIGINT,stop);std::signal(SIGTERM,stop);g_backend=bay_detect_backend();std::cout<<"\033[?25l";
    double hp=100,enemyHp=100,stamina=100,enemyStamina=100,gap=3,last=seconds(),nextEnemy=0,playerCooldown=0,guardUntil=0,enemyWind=0;
    int lateral=0,frame=0,rank=1,fightNo=1,wins=0,losses=0,groundControl=0,groundWins=0;bool strike=false,guard=false,groundMode=false,groundUnlocked=false,suddenDeathUsed=false;int totalWins=0,totalLosses=0;double damageToEnemy=0,sessionStart=0,aiNextAction=0;bool aiMode=true;int armsDealt=0,armsTaken=0,boutArms=0;char lawMsg[200];long lastReward=0;BayProfile prof;bay_profile_load(&prof);
#define BARBER_BREAK() do{double bt_=seconds();bay_barber_shop(&prof,g_backend,w,h,!aiMode);double el_=seconds()-bt_;nextEnemy+=el_;if(enemyWind>0)enemyWind+=el_;guardUntil+=el_;playerCooldown+=el_;aiNextAction+=el_;bay_joe_shift(el_);last=seconds();}while(0)
    std::string message="Double SPACE within 21 seconds for manual Player 1; otherwise Perfect Fighter.";
    std::cout<<"\033[2J\033[H\033[?25lBAYEISHUUI — PLAYER 1 SELECT\n\nDouble-tap SPACE within 21 seconds for manual play.\nOtherwise Perfect Fighter plays Player 1.\nPress ESC to cancel.\nMILITARY MARTIAL ARTS LAW IN FORCE: arms can break (B) - the bout fights on.\nPress H for the BARBER SHOP - hair styles from $800. Wallet: $"<<prof.money<<"\n"<<std::flush;
    double choiceStart=seconds(),lastSpace=-10.0;
    while(running&&seconds()-choiceStart<21.0){if(keyReady()){char ck=0;if(::read(STDIN_FILENO,&ck,1)==1){double kt=seconds();if(ck==27){running=false;break;}if(ck=='h'||ck=='H'){bay_barber_shop(&prof,g_backend,w,h,1);choiceStart=seconds();lastSpace=-10.0;std::cout<<"\033[2J\033[H\033[?25lBAYEISHUUI — PLAYER 1 SELECT\n\nDouble-tap SPACE within 21 seconds for manual play.\nOtherwise Perfect Fighter plays Player 1.\nPress ESC to cancel.\nMILITARY MARTIAL ARTS LAW IN FORCE: arms can break (B) - the bout fights on.\nPress H for the BARBER SHOP - hair styles from $800. Wallet: $"<<prof.money<<"\n"<<std::flush;continue;}if(ck==' '&&kt-lastSpace<=0.65){aiMode=false;break;}if(ck==' ')lastSpace=kt;}}
      std::cout<<"\033[8;1HStarting Perfect Fighter in "<<(21-static_cast<int>(seconds()-choiceStart))<<" seconds...  "<<std::flush;std::this_thread::sleep_for(std::chrono::milliseconds(50));}
    if(!running){std::cout<<"\nSession cancelled.\n";return 0;}sessionStart=seconds();last=sessionStart;
    message=aiMode?"PERFECT FIGHTER ACTIVE — live actions; ESC ends session.":"MANUAL PLAYER 1 — your controls are active.";
    bay_joe_start_bout();
    while(running&&hp>0&&rank<=15) {
        const int erank=bay_joe_rank_or(rank);
        bay_hair_sel[0]=bay_hair_rival_style(rank);bay_hair_lvl[0]=bay_hair_rival_level(rank);bay_hair_sel[1]=prof.equipped;bay_hair_lvl[1]=prof.level[prof.equipped];bay_hud_money=prof.money;
        double t=seconds(),dt=clamp(t-last,0,.12);last=t;strike=false;guard=t<guardUntil;
        if(keyReady()||aiMode){char k=0;bool gotKey=false;if(aiMode){if(keyReady()){char uk=0;ssize_t ur=::read(STDIN_FILENO,&uk,1);if(ur==1&&uk==27){running=false;message="ESCAPE — Perfect Fighter session ended.";}else if(ur==1&&(uk=='h'||uk=='H')){BARBER_BREAK();}}
 if(running&&t>=aiNextAction){k=bay_law_ai_action(hp,stamina,gap,lateral,groundMode,groundControl,enemyHp,bay_injury_mask[0],enemyWind,t,playerCooldown);if(!k){k=bayeishuui_ai_action(hp,enemyHp,stamina,gap,lateral,groundMode,groundUnlocked,groundControl,enemyWind,t,playerCooldown);if(k==' '&&!bay_law_can_punch(bay_injury_mask[1]))k='r';}gotKey=(k!='\0');aiNextAction=t+.22;}}
 else if(::read(STDIN_FILENO,&k,1)==1)gotKey=true;
 if(gotKey){
            if(k=='q'||k=='Q')break;
            if(k=='h'||k=='H'){BARBER_BREAK();}
            if(k=='w'||k=='W'){gap=clamp(gap-.22,.7,6.5);message="You step in behind your guard.";}
            if(k=='s'||k=='S'){gap=clamp(gap+.25,.7,6.5);message="You create space.";}
            if(k=='a'||k=='A'){lateral=static_cast<int>(clamp(lateral-1,-3,3));message="You circle left.";}
            if(k=='d'||k=='D'){lateral=static_cast<int>(clamp(lateral+1,-3,3));message="You circle right.";}
            if(k=='g'||k=='G'){guardUntil=t+.55;message="Guard up. Time the opponent.";}
            if((k=='b'||k=='B')&&t>=playerCooldown){
                if(groundMode&&groundControl<2)message="Arm lock needs ground control 2+ (J).";
                else if(!groundMode&&(gap>BAY_LAW_CLINCH_RANGE||std::abs(lateral)>2))message="Too far for an arm lock - close to clinch range.";
                else if(stamina<BAY_LAW_BREAK_STAMINA)message="Too tired to lock an arm.";
                else if(bay_injury_mask[0]==3)message="Both rival arms already broken - finish the fight.";
                else{
                    int chance=bay_law_break_chance(erank,groundControl,groundMode,enemyStamina);
                    stamina-=BAY_LAW_BREAK_STAMINA;playerCooldown=t+1.0;strike=true;
                    if(std::rand()%100<chance){int arm=bay_law_pick_arm(bay_injury_mask[0],std::rand());bay_injury_mask[0]=static_cast<unsigned char>(bay_injury_mask[0]|arm);enemyHp=clamp(enemyHp-bay_joe_snap_cost(BAY_LAW_BREAK_DAMAGE),0,100);++armsDealt;++boutArms;bay_joe_on_arm_snap(t);
                        std::snprintf(lawMsg,sizeof lawMsg,"CRACK! You break %s's %s arm! MMAL: the bout fights on.",bay_joe_name_or(opponents[rank-1]),bay_law_arm_name(arm));message=lawMsg;}
                    else{std::snprintf(lawMsg,sizeof lawMsg,"Arm lock slips free (%d%% chance). Reset and try again.",chance);message=lawMsg;}
                }
            }
            if(groundMode&&(k=='j'||k=='J')){++groundControl;message="Ground control: "+std::to_string(groundControl)+".";}
            else if(groundMode&&(k=='k'||k=='K')&&t>=playerCooldown&&stamina>=10){int dmg=static_cast<int>((12+std::rand()%7)*bay_law_damage_scale(bay_injury_mask[1])+0.5);enemyHp=clamp(enemyHp-bay_joe_dmg(dmg),0,100);if(enemyHp<=0)++groundWins;stamina-=10;playerCooldown=t+.7;strike=true;message="GROUND AND POUND! "+std::to_string(dmg)+" damage.";}
            else if(groundMode&&(k=='l'||k=='L')&&t>=playerCooldown&&groundControl>=2){if(std::rand()%100<35+groundControl*8){enemyHp=0;++groundWins;message="SUBMISSION! Bout finished.";}else{groundControl=0;playerCooldown=t+1.0;message="Submission defended; rebuild control.";}}
            else if(!groundMode&&k==' '&&!bay_law_can_punch(bay_injury_mask[1])){message="Both arms broken - fight on with teep (R) and round kick (E)!";}
            else if(!groundMode&&(k==' '||k=='e'||k=='E'||k=='r'||k=='R')&&t>=playerCooldown&&stamina>=13){
                double reach=(k=='r'||k=='R')?3.15:(k=='e'||k=='E')?2.7:2.25;
                int damage=(k=='r'||k=='R')?5:(k=='e'||k=='E')?11:8;
                const char* technique=(k=='r'||k=='R')?"TEEP":(k=='e'||k=='E')?"ROUND KICK":"STRAIGHT / ELBOW";
                stamina-=(k=='e'||k=='E')?19:(k=='r'||k=='R')?10:13;playerCooldown=t+((k=='e'||k=='E')?.78:.62);strike=true;
                if(gap<=reach&&std::abs(lateral)<=2){int dealt=damage+std::rand()%5;if(k==' ')dealt=static_cast<int>(dealt*bay_law_damage_scale(bay_injury_mask[1])+0.5);enemyHp=clamp(enemyHp-bay_joe_dmg(dealt),0,100);damageToEnemy+=dealt;message=std::string(technique)+" lands — return to Muay Thai guard.";if(!groundUnlocked&&damageToEnemy>=150){groundUnlocked=true;groundMode=true;groundControl=0;message="150% STRIKING DAMAGE! GROUND GAME UNLOCKED: J control, K ground-and-pound, L submission.";}}
                else message=std::string(technique)+" misses. Re-establish range.";
            }
        }}
        stamina=clamp(stamina+dt*(guard?3:9),0,100);enemyStamina=clamp(enemyStamina+dt*7,0,100);
        if(bay_joe_tick(t)){groundMode=true;groundControl=2;std::snprintf(lawMsg,sizeof lawMsg,"%s falls over! GROUND AND POUND (K) - or submit him (L)!",BAY_JOE_NAME);message=lawMsg;}
        if(t>=nextEnemy&&gap<3.0){
            double pace=(erank==1?1.0:(erank==2||erank==5||erank==7||erank==10?.72:(erank==3||erank==4||erank==8||erank==9||erank==11||erank==12||erank==13||erank==14?.9:.78)));
            enemyWind=t+(erank==3||erank==4||erank==8||erank==12||erank==14?.58:.40);
            nextEnemy=t+((1.65-erank*.055)*pace+(std::rand()%100)/100.0)*bay_law_enemy_tempo(bay_injury_mask[0]);
            if(erank==2||erank==5||erank==6||erank==7)gap=clamp(gap-.035,.7,6.5);
            if(erank==4||erank==8||erank==11||erank==14)lateral=(lateral==0?1:-lateral);
        }
        if(enemyWind>0&&t>=enemyWind){
            if(gap<=(erank==5||erank==6||erank==7?1.65:erank==9?2.7:2.1)&&std::abs(lateral)<=2){
                int base=(erank==4||erank==11||erank==13?10:erank==5||erank==7?8:6);
                if(guard){hp=clamp(hp-(1+erank/5)*bay_law_guard_penalty(bay_injury_mask[1]),0,100);message=std::string("CHECK / BLOCK! You absorb the ")+(erank%4==0?"straight punches":erank%4==1?"round kick":erank%4==2?"teep":"knee / elbow")+".";}
                else {bool broke=false;hp=clamp(hp-static_cast<int>((base+erank/2+std::rand()%7)*bay_law_damage_scale(bay_injury_mask[0])+0.5),0,100);
                    if(bay_injury_mask[1]!=3&&std::rand()%100<bay_law_enemy_break_chance(erank,stamina)){int arm=bay_law_pick_arm(bay_injury_mask[1],std::rand());bay_injury_mask[1]=static_cast<unsigned char>(bay_injury_mask[1]|arm);hp=clamp(hp-BAY_LAW_BREAK_DAMAGE/2,0,100);++armsTaken;broke=true;
                        std::snprintf(lawMsg,sizeof lawMsg,"SNAP! %s breaks your %s arm! MMAL: FIGHT ON - your legs still work!",bay_joe_name_or(opponents[rank-1]),bay_law_arm_name(arm));message=lawMsg;}
                    if(!broke)message=std::string(bay_joe_name_or(opponents[rank-1]))+" lands "+(erank%4==0?"straight punches":erank%4==1?"round kick":erank%4==2?"teep":"knee / elbow")+" — defend and counter.";}
            }
            else message="The opponent's strike misses at this range.";
            enemyWind=0;
        }
        if(gap>2.0&&frame%4==0)gap=clamp(gap-.025,.7,6.5);
        render(w,h,hp,enemyHp,stamina,enemyStamina,gap,w/2-3+lateral*2,w/2-2,guard,strike,message,frame++,rank,fightNo,wins,losses,groundMode,aiMode);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if(enemyHp<=0){bay_injury_mask[0]=bay_injury_mask[1]=0;++wins;++totalWins;++fightNo;lastReward=bay_profile_bout_reward(rank,boutArms,fightNo>17);prof.money+=lastReward;if(prof.money>BAY_MONEY_CAP)prof.money=BAY_MONEY_CAP;boutArms=0;bay_profile_save(&prof);groundMode=groundUnlocked;groundControl=0;enemyHp=100;enemyStamina=100;stamina=100;gap=3.2;lateral=0;enemyWind=0;nextEnemy=t+1.0;playerCooldown=t+.5;hp=100;bay_joe_start_bout();
            if(fightNo>17){int rating=80+groundWins*5;if(rating>100)rating=100;const char* kind=groundWins>=3?"FINISHER":(groundWins==0?"STRIKER":"BALANCED STRIKER / FINISHER");message=std::string(opponents[rank-1])+" rates you "+std::to_string(rating)+"/100 — "+kind+". SERIES BONUS +$"+std::to_string(lastReward);if(rank==15){rank=16;break;}++rank;fightNo=1;wins=0;damageToEnemy=0;groundUnlocked=false;groundWins=0;}else message="BOUT WON +$"+std::to_string(lastReward)+" — fight "+std::to_string(fightNo)+"/17, wins "+std::to_string(wins)+", losses "+std::to_string(losses)+".";
        }
        if(hp<=0){++losses;++totalLosses;if(!suddenDeathUsed&&totalWins>=11&&totalWins>totalLosses){suddenDeathUsed=true;hp=25;enemyHp=100;message="SUDDEN DEATH! Eligible: 11+ wins and ahead.";}else{running=false;message="KNOCKOUT — Sudden Death requires 11+ wins and a winning record.";}}
    }
    std::cout<<"\033[0m\033[?25h\n"<<(hp<=0?"Campaign over — you were defeated.":rank>15?"FIGHTER GUILD COMPLETE — all 15 opponents, 17 fights each!":"Campaign abandoned.")<<'\n'<<message<<'\n';
    bay_profile_save(&prof);
    std::cout<<"Wallet: $"<<prof.money<<"  Hair: "<<bay_hair_styles[prof.equipped].name<<"\n";
    std::cout<<"Military Martial Arts Law: arms broken dealt "<<armsDealt<<", taken "<<armsTaken<<".\n";
    bayeishuui_save_run_config(seconds()-sessionStart,rank>15,aiMode?1:0,totalWins,totalLosses);
    return 0;
}
