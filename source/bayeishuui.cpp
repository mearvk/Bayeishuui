#include <algorithm>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <thread>
#include <unistd.h>

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
void fighter(int x,int y,bool player,bool guarding,bool striking) {
    fg(player?100:255,player?210:105,player?255:115);
    std::cout<<"\033["<<y<<';'<<x<<"H";
    if(player) {
        std::cout<<"  O  \n\033["<<y+1<<';'<<x<<"H /|\\ \n\033["<<y+2<<';'<<x<<"H";
        std::cout<<(striking?"  |---->":" /|\\  ")<<"\n\033["<<y+3<<';'<<x<<"H / \\";
    } else {
        std::cout<<" O \n\033["<<y+1<<';'<<x<<"H"<<(striking?"-|-==>":" /|\\")<<"\n\033["<<y+2<<';'<<x<<"H / \\";
    }
    std::cout<<"\033[0m";
}
void render(int w,int h,double hp,double enemyHp,double stamina,double enemyStamina,double gap,int px,int ex,bool guarding,bool striking,const std::string& message,int frame) {
    int scene=std::clamp(h-8,10,24), horizon=scene/3;
    std::cout<<"\033[2J\033[H"; fg(130,205,255);
    std::cout<<"  BAYEISHUUI  /  TERMINAL FIGHT CLUB     ";fg(220,225,235);
    std::cout<<"ROUND 01   ";fg(255,195,100);std::cout<<"DISTANCE "<<static_cast<int>(gap*10)/10.0<<"m\n";
    bar("YOU ",hp,14,70,220,255);bar("RIVAL",enemyHp,14,255,90,105);std::cout<<'\n';
    bar("STAMINA",stamina,12,95,235,155);bar("RIVAL",enemyStamina,12,255,180,90);std::cout<<'\n';
    for(int y=0;y<scene;++y) {
        std::cout<<"\033["<<y+4<<";1H";
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
    fighter(ex,ey,false,false,false);fighter(px,py,true,guarding,striking);
    fg(255,210,130);std::cout<<"\033["<<h-3<<";1H "<<message;
    fg(175,190,210);std::cout<<"\033["<<h-1<<";1H W advance  A/D circle  S retreat  SPACE strike  G guard  Q quit";
    std::cout<<"\033[0m"<<std::flush;
}
}
int main() {
    if(!isatty(STDIN_FILENO)||!isatty(STDOUT_FILENO)){std::cerr<<"Run this game in an interactive Linux terminal.\n";return 2;}
    winsize ws{};ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws);int w=ws.ws_col?ws.ws_col:80,h=ws.ws_row?ws.ws_row:30;
    if(w<60||h<24){std::cerr<<"Please resize terminal to at least 60x24.\n";return 2;}
    if(tcgetattr(STDIN_FILENO,&original)<0){perror("tcgetattr");return 2;}
    termios raw=original;raw.c_lflag&=static_cast<tcflag_t>(~(ICANON|ECHO));raw.c_iflag&=static_cast<tcflag_t>(~(IXON|ICRNL));raw.c_cc[VMIN]=0;raw.c_cc[VTIME]=0;
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw)<0){perror("tcsetattr");return 2;}rawEnabled=true;
    std::atexit(restore);std::signal(SIGINT,stop);std::signal(SIGTERM,stop);std::cout<<"\033[?25l";
    double hp=100,enemyHp=100,stamina=100,enemyStamina=100,gap=3,last=seconds(),nextEnemy=0,playerCooldown=0,guardUntil=0,enemyWind=0;
    int lateral=0,frame=0;bool strike=false,guard=false;std::string message="Find your range. Watch the opponent's shoulders.";
    while(running&&hp>0&&enemyHp>0) {
        double t=seconds(),dt=clamp(t-last,0,.12);last=t;strike=false;guard=t<guardUntil;
        if(keyReady()) { char k=0;if(::read(STDIN_FILENO,&k,1)==1) {
            if(k=='q'||k=='Q')break;
            if(k=='w'||k=='W'){gap=clamp(gap-.22,.7,6.5);message="You step in behind your guard.";}
            if(k=='s'||k=='S'){gap=clamp(gap+.25,.7,6.5);message="You create space.";}
            if(k=='a'||k=='A'){lateral=static_cast<int>(clamp(lateral-1,-3,3));message="You circle left.";}
            if(k=='d'||k=='D'){lateral=static_cast<int>(clamp(lateral+1,-3,3));message="You circle right.";}
            if(k=='g'||k=='G'){guardUntil=t+.55;message="Guard up. Time the opponent.";}
            if(k==' '&&t>=playerCooldown&&stamina>=13){stamina-=13;playerCooldown=t+.62;strike=true;
                if(gap<=2.25&&std::abs(lateral)<=2){enemyHp=clamp(enemyHp-(7+std::rand()%9),0,100);message="CLEAN HIT — rival health drops.";}
                else message="Your strike falls short. Close distance first.";
            }
        }}
        stamina=clamp(stamina+dt*(guard?3:9),0,100);enemyStamina=clamp(enemyStamina+dt*7,0,100);
        if(t>=nextEnemy&&gap<3.0){enemyWind=t+.42;nextEnemy=t+1.25+(std::rand()%100)/100.0;}
        if(enemyWind>0&&t>=enemyWind){
            if(gap<=2.1&&std::abs(lateral)<=2){if(guard){hp=clamp(hp-2,0,100);message="BLOCK! You absorb most of the impact.";}
                else {hp=clamp(hp-(6+std::rand()%9),0,100);message="RIVAL CONNECTS — reset your guard.";}}
            else message="The opponent's strike misses at this range.";
            enemyWind=0;
        }
        if(gap>2.0&&frame%4==0)gap=clamp(gap-.025,.7,6.5);
        render(w,h,hp,enemyHp,stamina,enemyStamina,gap,w/2-3+lateral*2,w/2-2,guard,strike,message,frame++);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout<<"\033[0m\033[?25h\n"<<(hp<=0?"Bout over — you were knocked out.":enemyHp<=0?"Victory — opponent defeated.":"Bout abandoned.")<<'\n';
    return 0;
}
