#ifndef BAYEISHUUI_AI_H
#define BAYEISHUUI_AI_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
static char bayeishuui_ai_action(double hp,double enemy_hp,double stamina,double gap,int lateral,int ground_mode,int ground_unlocked,int ground_control,double enemy_wind,double t,double player_cooldown){
 if(hp<=0||enemy_hp<=0)return '\0';
 if(enemy_wind>t&&enemy_wind-t<0.50)return stamina>=4?'g':'s';
 if(ground_mode&&ground_unlocked){if(ground_control<2)return 'j';if(stamina>=10)return enemy_hp>18?'k':'l';return 'g';}
 if(stamina<18)return gap<2.4?'g':'s';
 if(abs(lateral)>2)return lateral>0?'a':'d';
 if(gap>3.0)return 'w';if(t<player_cooldown)return 'g';
 if(gap>2.7)return stamina>=10?'r':'w';if(gap>2.25)return stamina>=19?'e':'r';return stamina>=13?' ':'g';
}
static void bayeishuui_save_run_config(double duration,int victory,int ai_mode,int total_wins,int total_losses){
 const char *user=getenv("USER");char account[48];size_t j=0;if(!user||!*user)user="unknown";
 for(size_t i=0;user[i]&&j<sizeof(account)-1;++i){unsigned char c=(unsigned char)user[i];account[j++]=(((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='.')?(char)c:'-');}
 while(j&&(account[j-1]=='-'||account[j-1]=='.'))--j;account[j]='\0';if(!j)strcpy(account,"unknown");
 time_t now=time(NULL);struct tm *p=localtime(&now);if(!p)return;struct tm tmv=*p;char date[16];if(!strftime(date,sizeof(date),"%Y%m%d",&tmv))return;
 unsigned long long secs=duration>0?(unsigned long long)(duration+0.5):0,hours=secs/3600;unsigned mins=(unsigned)((secs/60)%60),ss=(unsigned)(secs%60);char dtag[40];snprintf(dtag,sizeof(dtag),"%lluh%02um%02us",hours,mins,ss);
 const char abc[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";srand((unsigned)(now^(time_t)getpid()));char token[9],name[192];FILE *f=NULL;
 for(int a=0;a<20;++a){for(int i=0;i<8;++i)token[i]=abc[rand()%(sizeof(abc)-1)];token[8]='\0';snprintf(name,sizeof(name),"%s_%s_%s_%s.cfg",token,date,dtag,account);f=fopen(name,"wx");if(f)break;}
 if(!f){fprintf(stderr,"\nCould not create unique fight config in current directory.\n");return;}
 fprintf(f,"{\n  \"format\": \"bayeishuui-fight-config-v1\",\n  \"created_at_local\": \"%04d-%02d-%02dT%02d:%02d:%02d\",\n  \"filename_duration\": \"%s\",\n  \"duration_seconds\": %llu,\n  \"result\": \"%s\",\n  \"player_1_mode\": \"%s\",\n  \"total_wins\": %d,\n  \"total_losses\": %d,\n  \"account\": \"%s\"\n}\n",tmv.tm_year+1900,tmv.tm_mon+1,tmv.tm_mday,tmv.tm_hour,tmv.tm_min,tmv.tm_sec,dtag,secs,victory?"final-victory":"session-ended",ai_mode?"perfect-fighter":"human",total_wins,total_losses,account);
 fclose(f);printf("\nFight record saved: %s\n",name);
}
#endif
