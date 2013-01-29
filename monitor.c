/*
Eagles Bulletin Board System
Copyright (C) 1995, Ray Rocker, rocker@datasync.com

EBBS Modules
Copyright (C) 1999, Paul Snow, psnow@nipha.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "modcommon.h"
#ifdef MOD_MONITOR

extern LOGININFO myinfo;     /* for idle timeout in Monitor */

extern struct shorturec *monitor_data;
extern int monitor_max;
extern int monitor_idle;
extern char global_modechar_key[];

extern NMENU *bigMenuList ;
extern NMENU *menuEnt[] ;
extern int currMenuEnt;

int monitormode;
int afkmode;

void
adv_monitor_refresh(sig)
{
  int i, boottime;
  if (sig) signal(sig, SIG_IGN);
  boottime = myinfo.idletimeout*120;
  if (boottime && ((monitor_idle += MONITOR_REFRESH) > boottime)) {
    disconnect(EXIT_TIMEDOUT);
  }
  if (bbs_check_mail()) {
    move(0, t_columns/3);
    prints("(You have mail.)");
  }
  if(monitormode)
    DoLongUserList();
  else
    DoShortUserList();
  for (i=0; i<global_ulist_sz; i++)
    if (!global_ulist[i].found) global_ulist[i].active = 0;

  signal(SIGALRM, adv_monitor_refresh);
  alarm(MONITOR_REFRESH);
}
int uoffset = 0;

DoLongUserList()
{
  USERDATA udata;
/*  ACCOUNT acct; */
  int i, maxucount, y = 2, x = 0, ucount = 0, lucount = 0;
  char tmpmodestr[256];
  time_t now;
  maxucount = t_lines - y - 2;
  for (i=0; i<global_ulist_sz; i++) {
    global_ulist[i].wasfound = global_ulist[i].found;
    global_ulist[i].found = 0;
  }
  move(y, x);
  clrtobot();
  time(&now);
  bbs_enum_users(global_ulist_sz, 0, NULL, FillShortUserList, NULL);
  ucount = 0;
  for (i=0; i<global_ulist_sz; i++)
    if (global_ulist[i].found) ucount++;
  if(ucount <= maxucount) uoffset = 0;
  if(uoffset) {
    int tmpcount;
    tmpcount = uoffset * maxucount;
    if((tmpcount + maxucount) > ucount)
      tmpcount = ucount - maxucount;
    if(tmpcount < 0) tmpcount = 0;
    lucount = tmpcount * -1;
    uoffset = 0;
    while (tmpcount > 0) {
      uoffset++;
      tmpcount = tmpcount - maxucount;
    }
  }
  else lucount = 0;
  prints("%-12s   %-25s %-25s   %s\n", "Userid", "Username","From","Mode");
  for (i=0; i<global_ulist_sz; i++) {
    if (global_ulist[i].found) {
      if (lucount >= 0 && lucount < maxucount) {
        utable_find_record(global_ulist[i].pid, &udata);
  /*      bbs_get_userinfo(global_ulist[i].userid, &acct); */
        sprintf(tmpmodestr, ModeToString(global_ulist[i].mode));
        tmpmodestr[9] = '\0';
        prints("%-12s %c %-25s %-25s %c %s\n", udata.u.userid, 
         BITISSET(global_ulist[i].flags, FLG_CLOAK) ? '#' : ' ',
         udata.u.username, udata.u.fromhost, 
         BITISSET(global_ulist[i].flags, FLG_NOPAGE) ? 'N' : ' ',
         tmpmodestr);
      }
      lucount++;
    }
  }
  move(t_lines-1, 0);
  prints("%d user%s online at %s", ucount, (ucount==1?"":"s"),Ctime(&now));
  move(t_lines-1,t_columns-18);
  prints("(S)witch Mon Mode");
  refresh();
  return ucount;
}

monitor_header()
{
  clear();
  prints("Monitor Mode                                        CTRL-C \
or CTRL-D to exit.\n");
  if(!monitormode)
    prints("%s\n", global_modechar_key);
/*
  prints("-----------------------------------------------------------\
------------------\n");
*/
  prints("---(A)fk (C)hat (T)alk (Q)uery (M)ail (P)ager (R)ead New (U\
)ser List (H)elp---\n");
} 

monitor_init(init)
int init;
{
  void (*asig)();
  int saved_alarm;
  if (init) 
  {
    monitor_idle = 0;
    if(afkmode)
      bbs_set_mode(M_AFK);
    else
      bbs_set_mode(M_MONITOR);
    saved_alarm = alarm(0);
    asig = signal(SIGALRM, SIG_IGN);
    adv_monitor_refresh(0);
  }
  else
  {
    alarm(0);
    signal(SIGALRM, asig);
    if (saved_alarm) alarm(saved_alarm);
    bbs_set_mode(M_UNDEFINED);
  }
}

NewMonitor()
{
  char ch;
  afkmode = 0;
#ifdef MOD_EXTRAS
  if (usermodcfg.monitor[0] == 'O')
    return Monitor();
  if (usermodcfg.monitor[0] == 'S')
    monitormode = 0;
  else
    monitormode = 1;
#else
  monitormode = 1;
#endif
  sprintf(global_modechar_key,
"Key: [M]ail [R]ead [P]ost [C]hat [m]on [T]alk [p]age Press: (S)wich Mon Mode");
  monitor_header();
  if (global_ulist == NULL) {
    if (SetupGlobalList() == -1) return PARTUPDATE;
  }
  monitor_init(1);
  while (1) {
    ch = MonitorGetch();
    monitor_idle = 0;
    if (ch == 0) break;
    if (ch == 1) 
      redoscr() ;
    else {
      if (ch == 2) {
        monitor_init(0);
        monitor_header();
        monitor_init(1);
      }
      else {
        global_modechar_key[0] = '\0';
        MonitorMenu(ch);
        sprintf(global_modechar_key,
"Key: [M]ail [R]ead [P]ost [C]hat [m]on [T]alk [p]age Press: (S)wich Mon Mode");
      }
    }
  }
  monitor_init(0);
  bbs_set_mode(M_UNDEFINED);
  memset(global_ulist, 0, global_ulist_sz * sizeof(*global_ulist));
  global_modechar_key[0] = '\0';
  return FULLUPDATE;
}

int
MonitorGetch()
{
  char c;
  int fieldsz = t_columns/3;
  while (1) {
    c = igetch();
    if (PagePending()) {
      monitor_init(0);
      Answer();
      monitor_header();
      monitor_init(1);
      return 1;
    }
    if (c == CTRL('L')) return 1;
    if (c == CTRL('C') || c == CTRL('D') || c == 'e' || c == 'E')
      return 0;
    if (c == 's' || c == 'S') {
      monitormode = abs(monitormode - 1);
      return 2;
    }
    if (c == 'a' || c == 'A') {
      afkmode = abs(afkmode - 1);
      return 2;
    }
    if (isprint(c)) return c;
    if (bbs_check_mail()) {
      move(0, fieldsz);
      prints("(You have mail.)");
    }
  }
}

/*ARGSUSED*/
int 
MonitorMenu(cmd)
char cmd ;
{
  int found = 0;
  NMENU *msp ;
  NMENUITEM *item = NULL;
  NMENUITEM *selitem;
  
  for(msp = bigMenuList;msp;msp=msp->next)
    if(!strcmp("Monitor",msp->menu_id))
      break ;
  if(!msp || currMenuEnt == MAXMENUDEPTH)
    return 0 ;
  currMenuEnt++ ;
  menuEnt[currMenuEnt] = msp ;
  selitem = msp->menucommands[GetMenuIndex(cmd)] ;
  if(selitem != NULL && HasPerm(selitem->enabled)) {
    item = msp->menucommands[GetMenuIndex(cmd)] ;
    if (item != NULL) {
      monitor_init(0);
      found = (item->action_func)(item->action_arg);
      monitor_header();
      monitor_init(1);
    }
  }
  currMenuEnt-- ;
  return found;
}

MonQuery()
{
  Query();
  pressreturn();
  return 1;
}

MonSetPager()
{
  SetPager();
  pressreturn();
  return 1;
}

MonOnlineUsers()
{
  OnlineUsers();
  pressreturn();
  return 1;
}

MonHelp()
{
  do_help();
  pressreturn();
  return 1;
}

MonToggleCloak()
{
#ifdef MOD_EXTRAS
  if(!mod_can_cloak()) {
    move(3,0);
    prints("\nSorry, General Purpose Cloak has been disabled.\n");
    clrtobot();
    pressreturn();
    return 1;
  }
#endif
  ToggleCloak();
  pressreturn();
  return 1;
}

#endif
